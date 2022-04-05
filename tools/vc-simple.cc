#include <iostream>

#include <torch/torch.h>
#include <torch/script.h>

torch::Tensor reload(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
  return torch::pickle_load(buffer).toTensor();
}

int main(int argc, const char* argv[]) {
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

  std::vector<torch::jit::IValue> inputs;

  using namespace torch::indexing;
  // Feature vector (2708, 1433)
  auto edgeIndex = reload(argv[2]);
  auto featureVectors = reload(argv[3]);
  auto groundTruth = reload(argv[4]);
  inputs.push_back(featureVectors);
  inputs.push_back(edgeIndex);

  int numVertices = featureVectors.size(0);

  auto options = torch::TensorOptions().dtype(torch::kBool);
  auto training_mask = torch::zeros({numVertices}, options);
  auto test_mask = torch::zeros({numVertices}, options);

  const int trainingSetSize = 500;
  const int testSetSize = 500;
  auto indices = torch::randint(0, numVertices, {trainingSetSize + testSetSize}, torch::TensorOptions().dtype(torch::kLong));
  training_mask.index_put_({indices.index({Slice(None, trainingSetSize)})}, true);
  test_mask.index_put_({indices.index({Slice(trainingSetSize, None)})}, true);

  std::cout << "Training set size : " << trainingSetSize
            << ", Test set size : " << testSetSize << std::endl;

  for (size_t epoch = 0; epoch < numEpochs; ++epoch) {
    model.train();
    // Forward step
    auto output = model.forward(inputs).toTensor();
    auto loss = torch::nn::functional::nll_loss(output.index({training_mask}), groundTruth.index({training_mask}));

    // Back-propagation
    optimizer.zero_grad();
    loss.backward();
    optimizer.step();

    // test
    model.eval();
    output = model.forward(inputs).toTensor();
    auto prediction = std::get<1>(output.max(1));
    auto equal = prediction.eq(groundTruth);
    int64_t test_correct = equal.index({test_mask}).sum().item<int64_t>();
    int64_t training_correct = equal.index({training_mask}).sum().item<int64_t>();

    std::cout << "Epoch : " << epoch + 1 << "/" << numEpochs << ", Train Accuracy: "
              << static_cast<float>(training_correct) / trainingSetSize
              << ", Test Accuracy: " << static_cast<float>(test_correct) / testSetSize
              << " | Loss: " << loss.item<double>() << std::endl;
  }
  return EXIT_SUCCESS;
}
