//===------------------------------------------------------------*- C++ -*-===//
//
//                            The AGILE Workflows
//
//===----------------------------------------------------------------------===//
// ** Pre-Copyright Notice
//
// This computer software was prepared by Battelle Memorial Institute,
// hereinafter the Contractor, under Contract No. DE-AC05-76RL01830 with the
// Department of Energy (DOE). All rights in the computer software are reserved
// by DOE on behalf of the United States Government and the Contractor as
// provided in the Contract. You are authorized to use this computer software
// for Governmental purposes but it is not to be released or distributed to the
// public. NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS
// OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. This
// notice including this sentence must appear on any copies of this computer
// software.
//
// ** Disclaimer Notice
//
// This material was prepared as an account of work sponsored by an agency of
// the United States Government. Neither the United States Government nor the
// United States Department of Energy, nor Battelle, nor any of their employees,
// nor any jurisdiction or organization that has cooperated in the development
// of these materials, makes any warranty, express or implied, or assumes any
// legal liability or responsibility for the accuracy, completeness, or
// usefulness or any information, apparatus, product, software, or process
// disclosed, or represents that its use would not infringe privately owned
// rights. Reference herein to any specific commercial product, process, or
// service by trade name, trademark, manufacturer, or otherwise does not
// necessarily constitute or imply its endorsement, recommendation, or favoring
// by the United States Government or any agency thereof, or Battelle Memorial
// Institute. The views and opinions of authors expressed herein do not
// necessarily state or reflect those of the United States Government or any
// agency thereof.
//
//                    PACIFIC NORTHWEST NATIONAL LABORATORY
//                                 operated by
//                                   BATTELLE
//                                   for the
//                      UNITED STATES DEPARTMENT OF ENERGY
//                       under Contract DE-AC05-76RL01830
//===----------------------------------------------------------------------===//

#include <chrono>
#include <iostream>
#include <iomanip>

#include "agile/workflow1/cora.h"

#include <torch/torch.h>
#include <torch/script.h>

torch::Tensor reload(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
  return torch::pickle_load(buffer).toTensor();
}

int main(int argc, char* argv[]) {
  if (argc != 5) {
    std::cerr << "usage: torch-test <path-to-exported-script-module> <edge_index> <feature_tensor> <class_tensor>\n";
    return EXIT_FAILURE;
  }

  torch::jit::script::Module model;
  try {
    model = torch::jit::load(argv[1]);
  } catch (const c10::Error & e) {
    std::cerr << "Error loading the module: " << e.msg() << std::endl;
    return EXIT_FAILURE;
  }

  std::vector<at::Tensor> parameters;
  for (const auto & params : model.parameters()) {
    parameters.push_back(params);
  }

  // Instantiate the optimizer.
  const double learningRate = 0.01;
  const int numEpochs = 200;
  torch::optim::Adam optimizer(parameters, torch::optim::AdamOptions(learningRate).weight_decay(5e-4));

  std::vector<torch::jit::IValue> inputs(2);

  using namespace torch::indexing;
  // Feature vector (2708, 1433)
  auto levels = torch::tensor({5,3,2,1});
  auto trainingSet = agile::CoraDataset(argv[2], argv[3], argv[4], levels);
  auto testSet = agile::CoraDataset(argv[2], argv[3], argv[4], levels);

  size_t numVertices = trainingSet.size().value();

  std::cout << "Num Vertices : " << numVertices << std::endl;

  auto options = torch::TensorOptions().dtype(torch::kBool);
  auto training_mask = torch::zeros({numVertices}, options);
  auto test_mask = torch::zeros({numVertices}, options);

  int64_t trainingSetSize = 500;
  int64_t testSetSize = 500;
  auto indices = torch::randint(0, numVertices, {trainingSetSize + testSetSize}, torch::TensorOptions().dtype(torch::kLong));
  training_mask.index_put_({indices.index({Slice(None, trainingSetSize)})}, true);
  test_mask.index_put_({indices.index({Slice(trainingSetSize, None)})}, true);

  trainingSetSize = training_mask.sum().item<int64_t>();
  testSetSize = test_mask.sum().item<int64_t>();

  std::cout.setf(std::ios_base::fixed, std::ios_base::floatfield);

  std::cout << "Training set size : " << trainingSetSize
            << ", Test set size : " << testSetSize
            << std::endl;

  auto stackedTrainSet = trainingSet.map(torch::data::transforms::Stack<agile::CoraData<>>());
  auto stackedTestSet = testSet.map(torch::data::transforms::Stack<agile::CoraData<>>());

  size_t batchSize = trainingSet.size().value() / 1;
  auto data_sampler = torch::data::samplers::DistributedSequentialSampler(trainingSet.size().value(),
                                                                          1, 0, false);
  auto data_loader = torch::data::make_data_loader(std::move(stackedTrainSet), data_sampler, batchSize);


  for (size_t epoch = 0; epoch < numEpochs; ++epoch) {
    auto start = std::chrono::high_resolution_clock::now();
    size_t train_correct = 0;
    size_t test_correct = 0;
    double total_loss = 0.0;
    for (auto &batch : *data_loader) {
      auto &input = batch.Features;
      auto & groundTruth = batch.Labels;

      model.train();

      inputs[0] = input;
      inputs[1] = batch.EdgeIndex;
      auto output = model.forward(inputs).toTensor();
      auto loss = torch::nn::functional::nll_loss(output.index({training_mask}), groundTruth.index({training_mask}));

      total_loss += loss.item<double>();

      optimizer.zero_grad();
      loss.backward();
      optimizer.step();

      model.eval();
      auto prediction = std::get<1>(output.max(1));
      auto equal = prediction.eq(groundTruth);
      train_correct += equal.index({training_mask}).sum().item<int64_t>();
      test_correct += equal.index({test_mask}).sum().item<int64_t>();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout
      << "Epoch : " << epoch + 1 << "/" << numEpochs << ", Train Accuracy: "
      << static_cast<float>(train_correct) / trainingSetSize
      << ", Test Accuracy: " << static_cast<float>(test_correct) / testSetSize
      << " | Loss: " << total_loss
      << " | Time (s) : " << std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count()
      << std::endl;
  }

  return EXIT_SUCCESS;
}
