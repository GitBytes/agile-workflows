/*===------------------------------------------------------------*- C++ -*-===
 *
 *                            The AGILE Workflows
 *
 *===----------------------------------------------------------------------===
 *
 * Copyright (c) 2025 Battelle Memorial Institute
 *
 * Battelle Memorial Institute (hereinafter Battelle) hereby grants permission
 * to any person or entity lawfully obtaining a copy of this software and
 * associated documentation files (hereinafter “the Software”) to redistribute
 * and use the Software in source and binary forms, with or without
 * modification. Such person or entity may use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and may permit
 * others to do so, subject to the following conditions:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Other than as used herein, neither the name Battelle Memorial Institute or
 *    Battelle may be used in any form whatsoever without the express written
 *    consent of Battelle.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL BATTELLE OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *===----------------------------------------------------------------------===*/
#include "agile/workflow1/gnn.h"
#include "agile/workflow1/graph.h"
#include "agile/workflow1/main.h"
#include "agile/workflow1/wmd.h"
#include "agile/workflow1/linkPrediction.h"

namespace agile::workflow1 {
using Emb_t = shad::Array<uint64_t>;
using EmbeddingType = shad::Array<uint64_t>;
using EmbeddingOID = shad::ObjectIdentifier<EmbeddingType>;

struct Args_t {
  uint64_t edgesOID;
  uint64_t embeddingsOID;
};

// Aggregate neigbors histograms ... store into second half of the feature
// vector
void TwoHopFeatures(Handle &handle, const uint64_t ndx, Vertex &vertex,
                    Args_t &args) {
  uint64_t num_edges = vertex.edges;
  if (num_edges == 0)
    return;

  auto Edges = XEdgeType::GetPtr((XEdgeOID)args.edgesOID);
  auto Embeddings = EmbeddingType::GetPtr((EmbeddingOID)args.embeddingsOID);

  Handle my_handle;
  std::vector<Edge> edges(num_edges);
  Edges->AsyncGetElements(my_handle, edges.data(), vertex.start, num_edges);

  waitForCompletion(my_handle);
  uint64_t num_bins = NUM_FEATURES / 2;
  std::vector<uint64_t> my_histogram(num_bins, 0);

  std::vector<uint64_t> neighbor_histograms(num_edges * num_bins);
  uint64_t *data = neighbor_histograms.data();

  for (uint64_t i = 0; i < num_edges; i++, data += num_bins) {
    uint64_t embedding_ndx = edges[i].dst_glbid * NUM_FEATURES;
    Embeddings->AsyncGetElements(my_handle, data, embedding_ndx, num_bins);
  }

  waitForCompletion(my_handle);
  for (uint64_t i = 0; i < num_edges * num_bins; i++)
    my_histogram[i % num_bins] += neighbor_histograms[i];

  uint64_t embedding_ndx =
      ndx * NUM_FEATURES +
      num_bins; // store aggregated histogram in second half
  Embeddings->AsyncInsertAt(handle, embedding_ndx, my_histogram.data(),
                            num_bins);
}

// Histogram edge and neigbor vertex types ... store in first half of feature
// vector
void OneHopFeatures(Handle &handle, const uint64_t ndx, Vertex &vertex,
                    Args_t &args) {
  uint64_t num_edges = vertex.edges;
  if (num_edges == 0)
    return;

  Handle my_handle;
  auto Edges = XEdgeType::GetPtr((XEdgeOID)args.edgesOID);
  auto Embeddings = EmbeddingType::GetPtr((EmbeddingOID)args.embeddingsOID);

  std::vector<Edge> edges(num_edges);
  Edges->AsyncGetElements(my_handle, edges.data(), vertex.start, num_edges);

  waitForCompletion(my_handle);
  uint64_t num_bins = NUM_FEATURES / 2;
  std::vector<uint64_t> histogram(num_bins, 0);

  for (auto &edge : edges) {
    histogram[(uint64_t)edge.type]++;
    histogram[(uint64_t)edge.dst_type]++;
  }

  Embeddings->AsyncInsertAt(handle, ndx * NUM_FEATURES, histogram.data(),
                            num_bins);
}

typename shad::Array<
    agile::workflow1::TrainingState<VertexClassificationWMDDataset>>::ObjectID
GCN(uint64_t &num_edges, uint64_t &num_vertices, Graph_t &graph,
    std::string modelFileName) {
  Handle handle;
  auto Vertices = VertexType::GetPtr((VertexOID)graph["Vertices"]);
  auto Embeddings = EmbeddingType::Create(num_vertices * NUM_FEATURES, 0);

  Embeddings->FillPtrs();
  graph["Embeddings"] = (uint64_t)(Embeddings->GetGlobalID());
  Args_t args = {graph["XEdges"], graph["Embeddings"]};

  Vertices->AsyncForEachInRange(handle, 0, num_vertices, OneHopFeatures, args);
  waitForCompletion(handle);

  Vertices->AsyncForEachInRange(handle, 0, num_vertices, TwoHopFeatures, args);
  waitForCompletion(handle);

  std::cout << "Embeddings created" << std::endl;

  size_t parallelThreads = shad::rt::numLocalities() * shad::rt::impl::getConcurrency();
  TrainingState<VertexClassificationWMDDataset> initState;
  auto TSs = shad::Array<TrainingState<VertexClassificationWMDDataset>>::Create(
      parallelThreads, initState);
  auto reducerArrayOID =
      shad::Array<uint64_t>::Create(parallelThreads, 0ul)
          ->GetGlobalID();
  auto localSamplesProcessedOID =
      shad::Array<uint64_t>::Create(parallelThreads, 0ul)
          ->GetGlobalID();
  auto localSamplesCorrectOID =
      shad::Array<uint64_t>::Create(parallelThreads, 0ul)
          ->GetGlobalID();
  SetUpTrainingContext<VertexClassificationWMDDataset> setup(
      Vertices->GetGlobalID(), (XEdgeOID)graph["XEdges"],
      Embeddings->GetGlobalID(), reducerArrayOID, localSamplesProcessedOID,
      localSamplesCorrectOID, modelFileName);
  // shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
  //                setup);
  TSs->ForEach([](size_t tid, TrainingState<VertexClassificationWMDDataset> & TS,
                  SetUpTrainingContext<VertexClassificationWMDDataset> &setup) {
    setup(tid, TS);
  }, setup);

  std::cout << "Initialized Training State" << std::endl;

  const size_t numEpochs = 10;
  auto localSamplesProcessedPtr =
      shad::Array<uint64_t>::GetPtr(localSamplesProcessedOID);
  auto localSamplesCorrectPtr =
      shad::Array<uint64_t>::GetPtr(localSamplesCorrectOID);
  for (size_t epoch = 0; epoch < numEpochs; ++epoch) {
    std::cout << "-- Epoch " << epoch + 1
              << " ------------------------------------------------------------"
                 "--------"
              << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
                   agile::workflow1::vcTrainLoop<
                       TrainingState<VertexClassificationWMDDataset>>);

    auto train_size = shad::reduce(shad::distributed_parallel_tag{},
                                   localSamplesProcessedPtr->begin(),
                                   localSamplesProcessedPtr->end());

    auto train_correct = shad::reduce(shad::distributed_parallel_tag{},
                                      localSamplesCorrectPtr->begin(),
                                      localSamplesCorrectPtr->end());

    std::cout << "Train Accuracy: " << train_correct << "/" << train_size
              << " = " << static_cast<float>(train_correct) / train_size
              << std::endl;
    if (shad::rt::numLocalities() > 1) {
      vcReduceGradients<TrainingState<VertexClassificationWMDDataset>>(
          TSs->begin(), TSs->end());
    }

    shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
                   agile::workflow1::vcBackPropAndEvaluationLoop<
                       TrainingState<VertexClassificationWMDDataset>>);
    auto test_size = shad::reduce(shad::distributed_parallel_tag{},
                                  localSamplesProcessedPtr->begin(),
                                  localSamplesProcessedPtr->end());

    auto test_correct = shad::reduce(shad::distributed_parallel_tag{},
                                     localSamplesCorrectPtr->begin(),
                                     localSamplesCorrectPtr->end());

    std::cout << "Test Accuracy: " << test_correct << "/" << test_size << " = "
              << static_cast<float>(test_correct) / test_size << std::endl;

    auto end = std::chrono::high_resolution_clock::now();

    std::cout << shad::rt::thisLocality() << " Time (s) : "
              << std::chrono::duration_cast<std::chrono::duration<double>>(
                     end - start)
                     .count()
              << std::endl;
  }

  std::cout << "Model Trained" << std::endl;

  return TSs->GetGlobalID();
}

typename shad::Array<
    agile::workflow1::lpTrainingState<LinkPredictionWMDDataset>>::ObjectID
LinkPredictor(uint64_t &num_edges, uint64_t &num_vertices, Graph_t &graph,
              std::string modelFileName) {

  std::cout << "PyTorch version: "
    << TORCH_VERSION_MAJOR << "."
    << TORCH_VERSION_MINOR << "."
    << TORCH_VERSION_PATCH << std::endl;

  Handle handle;
  auto Vertices = VertexType::GetPtr((VertexOID)graph["Vertices"]);
  auto Embeddings = EmbeddingType::Create(num_vertices * NUM_FEATURES, 0);

  Embeddings->FillPtrs();
  graph["Embeddings"] = (uint64_t)(Embeddings->GetGlobalID());
  Args_t args = {graph["XEdges"], graph["Embeddings"]};

  Vertices->AsyncForEachInRange(handle, 0, num_vertices, OneHopFeatures, args);
  waitForCompletion(handle);

  Vertices->AsyncForEachInRange(handle, 0, num_vertices, TwoHopFeatures, args);
  waitForCompletion(handle);

  std::cout << "Embeddings created" << std::endl;

  auto timeGenerateStart = my_timer();
  auto [observedGraph, trainSet, validationSet, testSet] = GenerateLinkPredictionDataSet(Vertices->GetGlobalID(), (XEdgeOID)graph["XEdges"], 0.85, 0.05);
  auto timeGenerateEnd = my_timer(); 
  std::cout<<"GenerateLinkPredictionDataSet time: " <<  timeGenerateEnd - timeGenerateStart << std::endl;

  size_t parallelThreads = shad::rt::numLocalities() * shad::rt::impl::getConcurrency();
  lpTrainingState<LinkPredictionWMDDataset> initState;

  auto TSs = shad::Array<lpTrainingState<LinkPredictionWMDDataset>>::Create(
      parallelThreads, initState);

  auto reducerArrayOID =
      shad::Array<uint64_t>::Create(parallelThreads, 0ul)
          ->GetGlobalID();
  auto localSamplesProcessedOID =
      shad::Array<uint64_t>::Create(parallelThreads, 0ul)
          ->GetGlobalID();
  auto localSamplesCorrectOID =
      shad::Array<float>::Create(parallelThreads, 0ul)
          ->GetGlobalID();
  lpSetUpTrainingContext<LinkPredictionWMDDataset> setup(
      Vertices->GetGlobalID(), (XEdgeOID)graph["XEdges"],
      Embeddings->GetGlobalID(), trainSet, testSet, validationSet, reducerArrayOID, localSamplesProcessedOID,
      localSamplesCorrectOID, modelFileName);

  TSs->ForEach([](size_t tid, lpTrainingState<LinkPredictionWMDDataset> & TS,
                  lpSetUpTrainingContext<LinkPredictionWMDDataset> &setup) {
    setup(tid, TS);
  }, setup);
  std::cout << "Initialized Training State" << std::endl;

  const size_t numEpochs = 10;
  auto localSamplesProcessedPtr =
      shad::Array<uint64_t>::GetPtr(localSamplesProcessedOID);
  auto localSamplesCorrectPtr =
      shad::Array<float>::GetPtr(localSamplesCorrectOID);

  for (size_t epoch = 0; epoch < numEpochs; ++epoch) {
    std::cout << "-- Epoch " << epoch + 1
              << " ------------------------------------------------------------"
                 "--------"
              << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
                   agile::workflow1::lpTrainLoop<
                       lpTrainingState<LinkPredictionWMDDataset>>);
    auto train_size = shad::reduce(shad::distributed_parallel_tag{},
                                   localSamplesProcessedPtr->begin(),
                                   localSamplesProcessedPtr->end());
    auto train_correct = shad::reduce(shad::distributed_parallel_tag{},
                                      localSamplesCorrectPtr->begin(),
                                      localSamplesCorrectPtr->end());

    std::cout << "Train Accuracy: " << train_correct << "/" << train_size
              << " = " << static_cast<float>(train_correct) / train_size
              << std::endl;
    //TODO fix for LP?
    if (shad::rt::numLocalities() > 1) {
      vcReduceGradients<lpTrainingState<LinkPredictionWMDDataset>>(
          TSs->begin(), TSs->end());
    }
//END OF TRAINING
    shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
                   agile::workflow1::lpValidateLoop<
                       lpTrainingState<LinkPredictionWMDDataset>>);

    auto val_size = shad::reduce(shad::distributed_parallel_tag{},
                                  localSamplesProcessedPtr->begin(),
                                  localSamplesProcessedPtr->end());

    auto val_correct = shad::reduce(shad::distributed_parallel_tag{},
                                     localSamplesCorrectPtr->begin(),
                                     localSamplesCorrectPtr->end());

    std::cout << "Validation Accuracy: " << val_correct << "/" << val_size << " = "
              << static_cast<float>(val_correct) / val_size << std::endl;

//END OF VALIDATION

    // shad::for_each(shad::distributed_parallel_tag{}, TSs->begin(), TSs->end(),
    //                agile::workflow1::lpTestLoop<
    //                    lpTrainingState<LinkPredictionWMDDataset>>);

    // auto test_size = shad::reduce(shad::distributed_parallel_tag{},
    //                               localSamplesProcessedPtr->begin(),
    //                               localSamplesProcessedPtr->end());

    // auto test_correct = shad::reduce(shad::distributed_parallel_tag{},
    //                                  localSamplesCorrectPtr->begin(),
    //                                  localSamplesCorrectPtr->end());

    // std::cout << "Test Accuracy: " << test_correct << "/" << test_size << " = "
    //           << static_cast<float>(test_correct) / test_size << std::endl;
//END OF TEST
    auto end = std::chrono::high_resolution_clock::now();

    std::cout << shad::rt::thisLocality() << " Time (s) : "
              << std::chrono::duration_cast<std::chrono::duration<double>>(
                     end - start)
                     .count()
              << std::endl;
  }

  std::cout << "Model Trained" << std::endl;
  return TSs->GetGlobalID();
}
} // namespace agile::workflow1
