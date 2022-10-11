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
#include <iomanip>
#include <iostream>

#include "agile/workflow1/cora.h"
#include "agile/workflow1/gnn.h"

#include <torch/script.h>
#include <torch/torch.h>

#include <shad/core/algorithm.h>
#include <shad/core/vector.h>
#include <shad/data_structures/array.h>

#include "mpi.h"

torch::Tensor reload(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
  return torch::pickle_load(buffer).toTensor();
}

using TrainingState = agile::workflow1::TrainingState<agile::CoraDataset>;

void getArg(int argc, char *argv[], int i, char *out) {
  size_t len = std::strlen(argv[i]);
  std::memcpy(out, argv[i], len);
  out[len] = '\0';
}

class SetUpFunctor {
  const int64_t trainingSetSize = 500;
  const int64_t testSetSize = 500;
  const size_t batchSize = 100;

public:
  SetUpFunctor(int argc, char *argv[]) {
    getArg(argc, argv, 1, modelFileName_);
    getArg(argc, argv, 2, edgeIndexFileName_);
    getArg(argc, argv, 3, featureVectorsFileName_);
    getArg(argc, argv, 4, labelFileName_);
  }

  void operator()(TrainingState &TS) {
    // Load Module
    TS.Module = torch::jit::load(modelFileName_);

    // Load Dataset
    auto levels = torch::tensor({5, 3, 2, 1});
    TS.DataSet = agile::CoraDataset(edgeIndexFileName_, featureVectorsFileName_,
                                    labelFileName_, levels);
    size_t numVertices = TS.DataSet.size().value();

    // Partition in Training/Test Set
    using namespace torch::indexing;
    auto options = torch::TensorOptions().dtype(torch::kBool);

    // Create DataLoader
    uint32_t thisLocality = static_cast<uint32_t>(shad::rt::thisLocality());
    auto train_sampler = torch::data::samplers::DistributedRandomSampler(
        trainingSetSize, shad::rt::numLocalities(), thisLocality, false);
    auto test_sampler = torch::data::samplers::DistributedRandomSampler(
        testSetSize, shad::rt::numLocalities(), thisLocality, false);
    auto stackedDataSet =
        TS.DataSet.map(torch::data::transforms::Stack<agile::CoraData<>>());
    TS.TrainDataLoader =
        torch::data::make_data_loader(stackedDataSet, train_sampler, batchSize);
    TS.TestDataLoader =
        torch::data::make_data_loader(stackedDataSet, test_sampler, batchSize);

    // Create Optimizer
    std::vector<at::Tensor> parameters;
    for (const auto &params : TS.Module.parameters()) {
      parameters.push_back(params);
    }

    const double learningRate = 0.01;
    const int numEpochs = 200;
    TS.Adam = std::make_unique<torch::optim::Adam>(
        parameters, torch::optim::AdamOptions(learningRate).weight_decay(5e-4));

    // Set inputs
    TS.Inputs.resize(2);
  }

  const char *modelFileName() const { return modelFileName_; }
  const char *edgeIndexFileName() const { return edgeIndexFileName_; }
  const char *featureVectorsFileName() const { return featureVectorsFileName_; }
  const char *labelFileName() const { return labelFileName_; }

private:
  char modelFileName_[256];
  char edgeIndexFileName_[256];
  char featureVectorsFileName_[256];
  char labelFileName_[256];
};

namespace shad {
int main(int argc, char *argv[]) {
  size_t parallelThreads = shad::rt::numLocalities();
  TrainingState initState;
  auto TSs = shad::Array<TrainingState>::Create(parallelThreads, initState);

  SetUpFunctor setUp(argc, argv);

  std::cout << "Loading module" << std::endl;
  shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end() - 1,
                 setUp);

  std::cout << "Setup done" << std::endl;

  const size_t numEpochs = 200;
  for (size_t epoch = 0; epoch < numEpochs; ++epoch) {
    shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end() - 1,
                   agile::workflow1::vcTrainLoop<TrainingState>);
  }

  return EXIT_SUCCESS;
}
} // namespace shad
