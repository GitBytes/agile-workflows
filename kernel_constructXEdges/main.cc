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

#include "main.h"

namespace shad {
  using namespace agile::kernel_constructXEdges;

struct Args_t { uint64_t delta; uint64_t oid; };


std::vector <std::string> split(std::string & line, char delim, uint64_t size = 0) {
  uint64_t ndx = 0, start = 0, end = 0;
  std::vector <std::string> tokens(size);

  for ( ; end < line.length(); end ++)  {
    if ( (line[end] == delim) || (line[end] == '\n') ) {
       tokens[ndx] = line.substr(start, end - start);
       start = end + 1;
       ndx ++;
  } }

  tokens[size - 1] = line.substr(start, end - start);     // flush last token
  return tokens;
}


void readFile(Handle & handle, const RF_args_t & args) {
  std::string line;
  struct stat stats;
  std::string filename = args.filename;
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  uint64_t num_locales = (uint64_t) shad::rt::numLocalities();

  std::ifstream file(filename);
  if (! file.is_open()) { printf("Locale %lu cannot open file %s\n", this_locale, filename.c_str()); exit(-1); }

  stat(filename.c_str(), & stats);

  uint64_t num_bytes = stats.st_size / num_locales;                      // file size / number of locales
  uint64_t start = this_locale * num_bytes;
  uint64_t end = start + num_bytes;

  file.seekg(start);
  if (start != 0) { getline(file, line); start += line.size() + 1; }     // discard partial line
  if (this_locale == num_locales - 1) end = stats.st_size;               // last locale processes to end of file

  auto Edges      = EdgeType::GetPtr      ((EdgeOID)       args.Edges_OID);
  auto GlobalIDS  = GlobalIDType::GetPtr ((GlobalIDOID)  args.GlobalIDS_OID);

  while (start < end) {
    getline(file, line);
    start += line.size() + 1;
    if (line[0] == '#') continue;                                // skip comments
    std::vector <std::string> tokens = split(line, ',', 10);     // delimiter and # tokens set for wmd data file

    if (tokens[0] == "Person") {
       uint64_t key = ENCODE<uint64_t, std::string, UINT>(tokens[1]);
       GlobalIDS->BufferedAsyncInsert(handle, key, Vertex(0, 0, TYPES::PERSON));
    } else if (tokens[0] == "ForumEvent") {
       uint64_t key = ENCODE<uint64_t, std::string, UINT>(tokens[4]);
       GlobalIDS->BufferedAsyncInsert(handle, key, Vertex(0, 0, TYPES::FORUMEVENT));
    } else if (tokens[0] == "Forum") {
       uint64_t key = ENCODE<uint64_t, std::string, UINT>(tokens[3]);
       GlobalIDS->BufferedAsyncInsert(handle, key, Vertex(0, 0, TYPES::FORUM));
    } else if (tokens[0] == "Publication") {
       uint64_t key = ENCODE<uint64_t, std::string, UINT>(tokens[5]);
       GlobalIDS->BufferedAsyncInsert(handle, key, Vertex(0, 0, TYPES::PUBLICATION));
    } else if (tokens[0] == "Topic") {
       uint64_t key = ENCODE<uint64_t, std::string, UINT>(tokens[6]);
       GlobalIDS->BufferedAsyncInsert(handle, key, Vertex(0, 0, TYPES::TOPIC));

    } else if (tokens[0] == "Sale") {
       Edge sale(tokens);
       Edges->BufferedAsyncInsert(handle, sale.src, sale);

       Edge purchase = sale;
       purchase.type = TYPES::PURCHASE;
       std::swap(purchase.src, purchase.dst);
       Edges->BufferedAsyncInsert(handle, purchase.src, purchase);

       GlobalIDS->BufferedAsyncInsert(handle, sale.src,     Vertex(0, 1, sale.src_type));
       GlobalIDS->BufferedAsyncInsert(handle, sale.dst,     Vertex(0, 0, sale.dst_type));
       GlobalIDS->BufferedAsyncInsert(handle, purchase.src, Vertex(0, 1, purchase.src_type));
       GlobalIDS->BufferedAsyncInsert(handle, purchase.dst, Vertex(0, 0, purchase.dst_type));

    } else if (tokens[0] == "Author") {
       Edge authors(tokens);
       Edges->BufferedAsyncInsert(handle, authors.src, authors);

       Edge writtenBY = authors;
       writtenBY.type = TYPES::WRITTENBY;
       std::swap(writtenBY.src, writtenBY.dst);
       std::swap(writtenBY.src_type, writtenBY.dst_type);
       Edges->BufferedAsyncInsert(handle, writtenBY.src, writtenBY);

       GlobalIDS->BufferedAsyncInsert(handle, authors.src,   Vertex(0, 1, authors.src_type));
       GlobalIDS->BufferedAsyncInsert(handle, authors.dst,   Vertex(0, 0, authors.dst_type));
       GlobalIDS->BufferedAsyncInsert(handle, writtenBY.src, Vertex(0, 1, writtenBY.src_type));
       GlobalIDS->BufferedAsyncInsert(handle, writtenBY.dst, Vertex(0, 0, writtenBY.dst_type));

    } else if (tokens[0] == "Includes") {
       Edge includes(tokens);
       Edges->BufferedAsyncInsert(handle, includes.src, includes);

       Edge includedIN = includes;
       includedIN.type = TYPES::INCLUDEDIN;
       std::swap(includedIN.src, includedIN.dst);
       std::swap(includedIN.src_type, includedIN.dst_type);
       Edges->BufferedAsyncInsert(handle, includedIN.src, includedIN);

       GlobalIDS->BufferedAsyncInsert(handle, includes.src,   Vertex(0, 1, includes.src_type));
       GlobalIDS->BufferedAsyncInsert(handle, includes.dst,   Vertex(0, 0, includes.dst_type));
       GlobalIDS->BufferedAsyncInsert(handle, includedIN.src, Vertex(0, 1, includedIN.src_type));
       GlobalIDS->BufferedAsyncInsert(handle, includedIN.dst, Vertex(0, 0, includedIN.dst_type));

    } else if (tokens[0] == "HasTopic") {
       Edge hasTopic(tokens);
       Edges->BufferedAsyncInsert(handle, hasTopic.src, hasTopic);

       Edge topicIN = hasTopic;
       topicIN.type = TYPES::TOPICIN;
       std::swap(topicIN.src, topicIN.dst);
       std::swap(topicIN.src_type, topicIN.dst_type);
       Edges->BufferedAsyncInsert(handle, topicIN.src, topicIN);

       GlobalIDS->BufferedAsyncInsert(handle, hasTopic.src, Vertex(0, 1, hasTopic.src_type));
       GlobalIDS->BufferedAsyncInsert(handle, hasTopic.dst, Vertex(0, 0, hasTopic.dst_type));
       GlobalIDS->BufferedAsyncInsert(handle, topicIN.src,  Vertex(0, 1, topicIN.src_type));
       GlobalIDS->BufferedAsyncInsert(handle, topicIN.dst,  Vertex(0, 0, topicIN.dst_type));

    } else if (tokens[0] == "HasOrg") {
       Edge hasOrg(tokens);
       Edges->BufferedAsyncInsert(handle, hasOrg.src, hasOrg);

       Edge orgIN = hasOrg;
       orgIN.type = TYPES::ORGIN;
       std::swap(orgIN.src, orgIN.dst);
       std::swap(orgIN.src_type, orgIN.dst_type);
       Edges->BufferedAsyncInsert(handle, orgIN.src, orgIN);

       GlobalIDS->BufferedAsyncInsert(handle, hasOrg.src, Vertex(0, 1, hasOrg.src_type));
       GlobalIDS->BufferedAsyncInsert(handle, hasOrg.dst, Vertex(0, 0, hasOrg.dst_type));
       GlobalIDS->BufferedAsyncInsert(handle, orgIN.src,  Vertex(0, 1, orgIN.src_type));
       GlobalIDS->BufferedAsyncInsert(handle, orgIN.dst,  Vertex(0, 0, orgIN.dst_type));
}  } }


// Update the global ids on this locale and spawn updateIDS on next locale.
void updateIDS(Handle & handle, const Args_t & args) {
  auto GlobalIDS = GlobalIDType::GetPtr((GlobalIDOID) args.oid);
  auto updateLambda = [] (const uint64_t & key, Vertex & value, const uint64_t & delta) {value.id += delta;};

  auto locale = (uint32_t) shad::rt::thisLocality();
  auto my_map = GlobalIDS->GetLocalHashmap();

  if (locale < shad::rt::numLocalities() - 1) {     // if not last locale, spawn updateIDS on next locale
    Args_t my_args = {args.delta + my_map->Size(), args.oid};
    shad::rt::asyncExecuteAt(handle, shad::rt::Locality(locale + 1), updateIDS, my_args);
  }

  my_map->ForEachEntry(updateLambda, args.delta);
}


// Fill in Vertices ... insert {key, value.edges, value.type} at index value.id
void moveVertex(Handle & handle, const uint64_t & key, Vertex & value, uint64_t & verticesOID) {
  auto Vertices = VertexType::GetPtr((VertexOID) verticesOID);
  Vertices->AsyncInsertAt(handle, value.id, Vertex(key, value.edges, value.type));
}


// Fill in dst global id and fire-and-forget insert
void Dst_(Handle & handle, const uint64_t & i, Vertex & dstV, uint64_t & ndx, Edge & edge, uint64_t & XEdgesOID) {
  auto XEdges = XEdgeType::GetPtr((XEdgeOID) XEdgesOID);

  edge.dst_glbid = dstV.id;
  XEdges->AsyncInsertAt(handle, ndx, edge);
};


// Move edges from Edges to XEdges
void moveEdges(Handle & handle, const uint64_t & src_id, std::vector<Edge> & edges,
     uint64_t & globalIDSOID, uint64_t & verticesOID, uint64_t & XEdgesOID) {
  auto Vertices  = VertexType::GetPtr((VertexOID) verticesOID);
  auto GlobalIDS = GlobalIDType::GetPtr((GlobalIDOID) globalIDSOID);

  Vertex srcVertex;
  GlobalIDS->Lookup(src_id, & srcVertex);                                    // lookup global id for src vertex
  uint64_t ndx = Vertices->At(srcVertex.id).start;                           // start index for src vertex edges

  for (auto edge : edges) {                                                  // for each edge of src vertex
    edge.src_glbid = srcVertex.id;
    GlobalIDS->AsyncApply(handle, edge.dst, Dst_, ndx, edge, XEdgesOID);     // ... send edge to dst vertex and forget
    ndx ++;                                                                  // ... increment edge index
} }


int main(int argc, char *argv[]) {
  Handle handle;
  double time1 = my_timer();
  std::string dataFile = argv[1];

  auto Edges     = EdgeType::Create(LARGE);
  auto GlobalIDS = GlobalIDType::Create(MEDIUM);

  RF_args_t args;
  args.Edges_OID     = (uint64_t) (Edges->GetGlobalID());
  args.GlobalIDS_OID = (uint64_t) (GlobalIDS->GetGlobalID());
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);

  shad::rt::asyncExecuteOnAll(handle, readFile, args);
  waitForCompletion(handle);

  Edges->WaitForBufferedInsert();
  GlobalIDS->WaitForBufferedInsert();

/********** CREATE COMPRESSED EDGE ARRAY AND VERTEX ARRAY **********/
// ***** convert local ids to global ids *****/
  Args_t my_args = {0, (uint64_t) (GlobalIDS->GetGlobalID())};
  shad::rt::asyncExecuteAt(handle, shad::rt::Locality(0), updateIDS, my_args);

// ***** allocate space for Compressed Edges and Vertices *****/
  uint64_t num_edges = Edges->Size();
  uint64_t num_vertices = GlobalIDS->Size();
  auto XEdges = XEdgeType::Create(num_edges, Edge());
  auto Vertices = VertexType::Create(num_vertices + 1, Vertex());

  XEdges->FillPtrs();
  Vertices->FillPtrs();
  waitForCompletion(handle);

  uint64_t XEdges_OID    = (uint64_t) (XEdges->GetGlobalID());
  uint64_t vertices_OID  = (uint64_t) (Vertices->GetGlobalID());
  uint64_t globalIDS_OID = (uint64_t) (GlobalIDS->GetGlobalID());

// ***** copy vertices from GlobalIDS to Vertices *****/
  GlobalIDS->AsyncForEachEntry(handle, moveVertex, vertices_OID);
  waitForCompletion(handle);

  exclusiveScanVertices<Vertex>(vertices_OID);     // convert # edges to start location

// ***** move edges from Edges to XEdges *****/
  Edges->AsyncForEachEntry(handle, moveEdges, globalIDS_OID, vertices_OID, XEdges_OID);
  waitForCompletion(handle);

  printf("Time for CSR construction = %lf\n\n", my_timer() - time1);
  printf("Total number of edges    = %lu\n", num_edges);
  printf("Total number of vertices = %lu\n", num_vertices);

// ***** write out data structures ***** //

// print Global IDS
// print Vertices array
// print XEdges array

  return 0;
}

}
