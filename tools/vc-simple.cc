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
  auto trainingSet = agile::CoraDataset(argv[2], argv[3], argv[4]);
  auto testSet = agile::CoraDataset(argv[2], argv[3], argv[4]);

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

  auto stackedTrainSet = trainingSet.map(torch::data::transforms::Stack<>());
  auto stackedTestSet = testSet.map(torch::data::transforms::Stack<>());

  size_t batchSize = trainingSet.size().value() / 1;
  auto data_sampler = torch::data::samplers::DistributedSequentialSampler(trainingSet.size().value(),
                                                                          1, 0, false);
  auto data_loader = torch::data::make_data_loader(std::move(stackedTrainSet), data_sampler, batchSize);

  inputs[1] = trainingSet.edge_index();

  for (size_t epoch = 0; epoch < numEpochs; ++epoch) {
    auto start = std::chrono::high_resolution_clock::now();
    size_t train_correct = 0;
    size_t test_correct = 0;
    double total_loss = 0.0;
    for (auto &batch : *data_loader) {
      auto input = batch.data;
      auto groundTruth = batch.target;

      model.train();

      inputs[0] = input;
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
