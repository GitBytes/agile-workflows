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
#include <set>
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
  uint64_t max_neighbors = levels[level - 1];

  std::random_device rd;
  std::mt19937 g(rd());

  std::vector<Edge> neighborhood;
  neighborhood.reserve(levels[0]);
  while (level < levels.size()) {
    if (next == end_of_level)
      break; // BFS is exhausted

    uint64_t glbID = *(next++);   // advance frontier
    Vertex V = vertex_set[glbID]; // get next vertex
    uint64_t V_localID = V.id;    // get V's local id

    uint64_t startEL = V.start; // get V's neighbor list
    uint64_t endEL = startEL + V.edges;
    uint64_t num_neighbors = endEL - startEL;

    bool not_last_level = level < (levels.size() - 1);
    if (num_neighbors != 0 && (not_last_level || vertex_set.find(glbID) != vertex_set.end())) {
      uint64_t edges_to_fetch =
          std::min<uint64_t>(levels[level], num_neighbors);
      neighborhood.resize(edges_to_fetch);

      std::uniform_int_distribution<int> D(0, num_neighbors - 1);
      for (int i = 0; i < edges_to_fetch; ++i) {
        size_t v = D(g);
        Edges->AsyncGetElements(handle, &neighborhood[i], startEL + v, 1);
      }

      shad::rt::waitForCompletion(handle);
    }

    for (uint64_t i = 0; i < neighborhood.size(); ++i) {
      uint64_t uGlbID = neighborhood[i].dst_glbid;
      Vertex U = Vertices->At(uGlbID);

      // The last level is just a fake to cover a corner case.
      bool not_visited = vertex_set.find(uGlbID) == vertex_set.end();
      bool visited = !not_visited;
      if (not_last_level && not_visited) { // U is not visited

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
        if (not_last_level || visited) {
          uint64_t U_localID = vertex_set[uGlbID].id; // ... get U's local id
          edges.insert(std::make_pair(
              V_localID, U_localID)); // ... insert V-U edge into edge set
          edges.insert(std::make_pair(
              U_localID, V_localID)); // ... insert U-V edge into edge set
        }
      }
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
} // namespace agile::workflow1
