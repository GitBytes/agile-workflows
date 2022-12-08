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

#include <cstddef>
#include <cstdint>
#include <random>
#include <torch/csrc/autograd/generated/variable_factories.h>

#include "agile/workflow1/graph.h"
#include "agile/workflow1/utils.h"
#include "agile/workflow1/wmd.h"

#include "shad/core/algorithm.h"
#include "shad/runtime/runtime.h"

namespace agile::workflow1 {

std::tuple<torch::Tensor, std::map<uint64_t, Vertex>>
WMDDataset::_build_ego_graph(int64_t *rootB, int64_t *rootE) {
  shad::rt::Handle handle;
  auto Edges = XEdgeType::GetPtr(_edgesOID);
  auto Vertices = VertexType::GetPtr(_verticesOID);

  using namespace torch::indexing;
  auto bool_tensor = torch::TensorOptions().dtype(torch::kBool);

  uint64_t localID = 0;
  std::deque<uint64_t> frontier;
  std::map<uint64_t, Vertex> vertex_set;
  std::vector<uint64_t> levels{5, 3, 2, 1,
                               0}; // last 0 required to flush frontier
  std::set<std::pair<uint64_t, uint64_t>> edges;

  for (int64_t root = *rootB; rootB < rootE; root = *(++rootB)) {
    Vertex V = Vertices->At(root);  // get root
    uint64_t V_localID = localID++; // get next local ID

    V.id = V_localID;         // assign V a local ID
    vertex_set[root] = V;     // insert V into vertex set
    frontier.push_back(root); // push V's global id onto frontier
    edges.insert(
        std::make_pair(V_localID, V_localID)); // insert self edge into edge set
  }

  uint64_t level = 1; // level 0 was just consumed in the previous block
  auto next = frontier.begin();
  auto end_of_level = frontier.end();
  uint64_t added_neighbors = 0;
  uint64_t max_neighbors = levels[level - 1];

  std::random_device rd;
  std::mt19937 g(rd());

  while (level < levels.size()) {
    if (next == end_of_level)
      break; // BFS is exhausted

    uint64_t glbID = *(next++);   // advance frontier
    Vertex V = vertex_set[glbID]; // get next vertex
    uint64_t V_localID = V.id;    // get V's local id

    uint64_t startEL = V.start; // get V's neighbor list
    uint64_t endEL = startEL + V.edges;
    uint64_t num_neighbors = endEL - startEL;

    std::vector<Edge> neighborhood;
    if (num_neighbors != 0 && (level < (levels.size() - 1) ||
                               vertex_set.find(glbID) != vertex_set.end())) {
      neighborhood.resize(levels[level]);

      std::uniform_int_distribution<int> D(0, num_neighbors - 1);
      for (int i = 0; i < levels[level]; ++i) {
        size_t v = D(g);
        Edges->AsyncAt(handle, startEL + v, &neighborhood[i]);
      }

      shad::rt::waitForCompletion(handle);
    }

    added_neighbors = 0;
    for (uint64_t i = 0; i < neighborhood.size(); ++i) {
      uint64_t uGlbID = neighborhood[i].dst_glbid;
      Vertex U = Vertices->At(uGlbID);

      if (level <
              (levels.size() -
               1) && // The last level is just a fake to cover a corner case.
          vertex_set.find(uGlbID) == vertex_set.end()) { // U is not visited
        if (added_neighbors >= max_neighbors)
          continue; // ... if no more neighbors to add, continue

        uint64_t U_localID = localID++; // ... get next local id

        U.id = U_localID;           // ... assign U a local id
        vertex_set[uGlbID] = U;     // ... insert U into vertex set
        frontier.push_back(uGlbID); // ... push U's global id onto frontier
        edges.insert(std::make_pair(
            U_localID, U_localID)); // ... insert self edge into edge set
        edges.insert(std::make_pair(
            V_localID, U_localID)); // ... insert V-U edge into edge set
        edges.insert(std::make_pair(
            U_localID, V_localID)); // ... insert U-V edge into edge set
      } else {                      // U is visited
        if (level < (levels.size() - 1) ||
            vertex_set.find(uGlbID) != vertex_set.end()) {
          uint64_t U_localID = vertex_set[uGlbID].id; // ... get U's local id
          edges.insert(std::make_pair(
              V_localID, U_localID)); // ... insert V-U edge into edge set
          edges.insert(std::make_pair(
              U_localID, V_localID)); // ... insert U-V edge into edge set
        }
      }
      added_neighbors++;
    }

    if (next == end_of_level) { // go to next level
      level++;
      max_neighbors = levels[level - 1];
      end_of_level = frontier.end();
    }
  }

  int64_t num_edges = edges.size();
  int64_t num_vertices = vertex_set.size();

  // create source and destination vectors
  std::vector<int64_t> sources;
  std::vector<int64_t> destinations;

  for (auto &edge : edges) {
    sources.push_back((int64_t)edge.first);
    destinations.push_back((int64_t)edge.second);
  }

  // The result tensor stores the edge list of the ego-graph.
  auto options = torch::TensorOptions().dtype(torch::kLong);
  auto result = torch::zeros({2, num_edges}, options);

  result.slice(0, 0, 1) =
      torch::from_blob(sources.data(), {num_edges}, options).clone();
  result.slice(0, 1, 2) =
      torch::from_blob(destinations.data(), {num_edges}, options).clone();

  return std::make_tuple(result, vertex_set);
}

struct EndPointsEdgeCompare {
  bool operator()(const Edge *e1, const Edge *e2) const {
    return (e1->src_glbid == e2->src_glbid && e1->dst_glbid == e2->dst_glbid) ||
           (e1->src_glbid == e2->dst_glbid && e1->dst_glbid == e2->src_glbid);
  }
};

auto GenerateFalseEdges(
    size_t numEdges,
    typename shad::Set<Edge, EndPointsEdgeCompare>::SharedPtr Edges,
    size_t numVertices) {
  auto result = XEdgeType::Create(numEdges, Edge());

  auto EdgesOID = Edges->GetGlobalID();
  shad::generate(
      shad::distributed_parallel_tag{}, result->begin(), result->end(), [=]() {
        auto Edges = shad::Set<Edge, EndPointsEdgeCompare>::GetPtr(EdgesOID);

        std::random_device rd;
        std::default_random_engine G(rd());
        std::uniform_int_distribution<uint64_t> dist(0,
                                                     numVertices * numVertices);

        Edge e;
        do {
          // throw a dart in the matrix
          uint64_t idx = dist(G);
          e.src_glbid = idx / numVertices;
          e.dst_glbid = idx % numVertices;
          if (e.src_glbid > e.dst_glbid)
            std::swap(e.src_glbid, e.dst_glbid);
        } while (Edges->Find(e));
        // found a missing edge
        return e;
      });
  return result;
}

void GenerateLinkPredictionDataSet(Graph_t &graph, float train,
                                   float validation) {
  auto Vertices = VertexType::GetPtr((VertexOID)graph["Vertices"]);
  auto allEdges = XEdgeType::GetPtr((XEdgeOID)graph["XEdges"]);

  // Get lower triangular part
  shad::rt::Handle h = shad::rt::impl::createHandle();
  using EdgeSetType = shad::Set<Edge, EndPointsEdgeCompare>;
  auto EdgeSet = EdgeSetType::Create(allEdges->Size() / 2);
  auto EdgeSetOID = EdgeSet->GetGlobalID();

  allEdges->AsyncForEach(
      h,
      [](shad::rt::Handle &h, size_t i, Edge &e,
         EdgeSetType::ObjectID &EdgeSetOID) {
        if (e.src_glbid < e.dst_glbid) {
          auto set = EdgeSetType::GetPtr(EdgeSetOID);
          set->AsyncInsert(h, e);
        }
      },
      EdgeSetOID);
  shad::rt::waitForCompletion(h);

  auto Edges = XEdgeType::Create(EdgeSet->Size(), Edge());
  copy(shad::distributed_parallel_tag{}, EdgeSet->begin(), EdgeSet->end(),
       Edges->begin());

  size_t trueTrainEdgesNum = std::floor(Edges->Size() * train);
  size_t trueValidationEdgesNum = std::floor(Edges->Size() * validation);
  size_t trueTestEdgesNum =
      Edges->Size() - trueTrainEdgesNum - trueValidationEdgesNum;

  size_t falseEdgesTotal =
      std::min((Vertices->Size() - 1) * (Vertices->Size() - 1), Edges->Size());
  size_t falseTrainEdgesNum = std::floor(falseEdgesTotal * train);
  size_t falseValidationEdgesNum = std::floor(falseEdgesTotal * validation);
  size_t falseTestEdgesNum =
      falseEdgesTotal - falseTestEdgesNum - falseValidationEdgesNum;

  auto trainingSet =
      XEdgeType::Create(trueTrainEdgesNum + falseTrainEdgesNum, Edge());
  auto validationSet = XEdgeType::Create(
      trueValidationEdgesNum + falseValidationEdgesNum, Edge());
  auto testSet =
      XEdgeType::Create(trueTestEdgesNum + falseTestEdgesNum, Edge());

  // TODO: We need a random shuffle to scramble the order of edges before
  //       assigning it to one of train, validation, and test.
  auto begin = Edges->begin();
  auto end = begin + trueTrainEdgesNum;
  auto falseTrainItrB =
      copy(shad::distributed_parallel_tag{}, begin, end, trainingSet->begin());

  begin = end;
  end += trueValidationEdgesNum;
  auto falseValItrB = copy(shad::distributed_parallel_tag{}, begin, end,
                           validationSet->begin());

  begin = end;
  end += trueTestEdgesNum;
  auto falseTestItrB =
      copy(shad::distributed_parallel_tag{}, begin, end, testSet->begin());

  // Generate False Edges
  auto falseEdgeArray =
      GenerateFalseEdges(falseEdgesTotal, EdgeSet, Vertices->Size() - 1);
  begin = falseEdgeArray->begin();
  end = begin + falseTrainEdgesNum;
  copy(shad::distributed_parallel_tag{}, begin, end, falseTrainItrB);

  begin = end;
  end += falseValidationEdgesNum;
  copy(shad::distributed_parallel_tag{}, begin, end, falseValItrB);

  begin = end;
  end += falseTestEdgesNum;
  copy(shad::distributed_parallel_tag{}, begin, end, falseValItrB);

  // Create Observed Graph
  Graph_t observedGraph;

  // Freeing up temporaries
  EdgeSetType::Destroy(EdgeSetOID);
  XEdgeType::Destroy(Edges->GetGlobalID());
  // return std::make_tuple(observedGraph, trainingSet, validationSet, testSet);
}
} // namespace agile::workflow1
