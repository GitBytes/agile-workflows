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
#include "agile/workflow1/main.h"
#include "agile/workflow1/graph.h"
#include "agile/workflow1/linkPrediction.h"

namespace shad {
  using namespace agile::workflow1;

int main(int argc, char *argv[]) {
  double time1 = my_timer();

  Handle handle;
  Graph_t graph;
  std::string dataFile = argv[1];
  auto Edges  = EdgeType::Create(AGILE_LARGE);
  auto GlobalIDS = GlobalIDType::Create(AGILE_MEDIUM);
  graph["Edges"] = (uint64_t) (Edges->GetGlobalID());
  graph["GlobalIDS"] = (uint64_t) (GlobalIDS->GetGlobalID());

  RF_args_t args;
  args.Edges_OID     = graph["Edges"];
  args.GlobalIDS_OID = graph["GlobalIDS"];
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);

  shad::rt::asyncExecuteOnAll(handle, readFile, args);
  waitForCompletion(handle);

  Edges->WaitForBufferedInsert();
  GlobalIDS->WaitForBufferedInsert();

/********** CREATE COMPRESSED EDGE ARRAY AND VERTEX ARRAY **********/
  uint64_t num_vertices = GlobalIDS->Size();
  uint64_t num_edges = Edges->Size();

  CSR(graph, num_vertices, num_edges);
  printf("Time for graph construction = %lf\n", my_timer() - time1);
  printf("Total number of vertices = %lu\n", num_vertices);
  printf("Total number of edges    = %lu\n", num_edges);

  time1 = my_timer();

  auto trainedModel = LinkPredictor(num_edges, num_vertices, graph, argv[2]);
  printf("Time for wk1_linkpred = %lf\n", my_timer() - time1);

  return 0;
}

}
