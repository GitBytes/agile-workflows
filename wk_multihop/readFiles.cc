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
#include <limits>
#include <string>
#include <cstdlib>
#include <sys/stat.h>

#include "agile/wk_multihop/main.h"
#include "agile/wk_multihop/graph.h"
// #include "agile/wk_multihop/common.h"

#define EMBEDDING_DIM 768

namespace agile::wk_multihop {

Graph_t graph;

std::vector <std::uint64_t> split(std::string & line, char delim, uint64_t size = 0) {
  uint64_t ndx = 0, start = 0, end = 0;
  std::vector <std::uint64_t> tokens(size);

  for ( ; end < line.length(); end ++)  {
    if ( (line[end] == delim) || (line[end] == '\n') ) {
      tokens[ndx] = std::stoull(std::string(line.substr(start, end - start)));
      start = end + 1;
      ndx ++;
    } }

  tokens[size - 1] = std::stoull(std::string(line.substr(start, end - start)));     // flush last token
  return tokens;
}

// template<typename T>
std::vector <float> split_embedding(std::string & line, char delim, uint64_t size = 0) {
  uint64_t ndx = 0, start = 0, end = 0;
  std::vector <float> tokens(size);

  for ( ; end < line.length(); end ++)  {
    if ( (line[end] == delim) || (line[end] == '\n') ) {
      tokens[ndx] = std::stof(std::string(line.substr(start, end - start)));
      start = end + 1;
      ndx ++;
    } }
  // flush last token
  tokens[size - 1] = std::stof(std::string(line.substr(start, end - start))); 
  // if (ndx < 768 ) std::cout << "COUNT " << ndx << std::endl;
  return tokens;
}


void readEdgeFile(Handle & handle, const RF_args_t & args) {
  // std::cout << "In readfile, reading edge data file " 
  // << args.edge_data_filename << std::endl;
  std::string line;
  struct stat stats;
  std::string filename = args.edge_data_filename;
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  uint64_t num_locales = (uint64_t) shad::rt::numLocalities();

  std::ifstream file(filename);
  if (! file.is_open()) { 
    printf("Locale %lu cannot open file %s\n", this_locale, filename.c_str()); exit(-1); 
  }

  stat(filename.c_str(), & stats);

  uint64_t num_bytes = stats.st_size / num_locales;             // file size / number of locales
  uint64_t start = this_locale * num_bytes;
  uint64_t end = start + num_bytes;

  if (this_locale != 0) {                                       // check for partial line
    file.seekg(start - 1);
    getline(file, line);
    if (line[0] != '\n') start += line.size();                 // if not at start of a line, discard partial line
  }

  if (this_locale == num_locales - 1) end = stats.st_size;      // last locale processes to end of file


  auto count = 0;
  while (start < end) {
    getline(file, line);
    start += line.size() + 1;
    if (count == 0 && this_locale == 0) {count++; continue; /*skip first line*/}
    std::vector <std::uint64_t> tokens = split(line, ',', 4);     // delimiter and # tokens set for wikidata data file

    auto edge_type_token = tokens[2]; /*0,0,167,2648053*/

    auto CurrentEdgeTable = WikiDataEdgeType::GetPtr( (WikiDataEdgeOID) 
    						      graph[edge_type_token]);
    WikiDataEdge record(tokens);
    CurrentEdgeTable->BufferedAsyncInsert(handle, record.key(), record);
    count++;
  }

  file.close();
}


void readEntityEmbeddingFile(Handle & handle, const RF_args_t & args) {
  std::string line;
  struct stat stats;
  std::string filename = args.entity_embedding_filename;
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  uint64_t num_locales = (uint64_t) shad::rt::numLocalities();

  std::ifstream file(filename);
  if (! file.is_open()) { 
    printf("Locale %lu cannot open file %s\n", this_locale, filename.c_str()); exit(-1); 
  }

  stat(filename.c_str(), & stats);

  uint64_t num_bytes = stats.st_size / num_locales;             // file size / number of locales
  uint64_t start = this_locale * num_bytes;
  uint64_t end = start + num_bytes;

  if (this_locale != 0) {                                       // check for partial line
    file.seekg(start - 1);
    getline(file, line);
    if (line[0] != '\n') start += line.size();                 // if not at start of a line, discard partial line
  }

  if (this_locale == num_locales - 1) end = stats.st_size;      // last locale processes to end of file


  auto count = 0;
  
  auto EntityEmbeddingTable = EntityEmbeddingType::GetPtr( (EntityEmbeddingOID) 
    						      args.entity_embedding_table_OID);
  while (start < end) {
    getline(file, line);
    start += line.size() + 1;
    if (count == 0 && this_locale == 0) {count++; continue; /*skip first line*/}
    
    auto id = std::stoull (line.substr(0, line.find_first_of(",")));

    std::vector <float> tokens = split_embedding(line, ',', EMBEDDING_DIM + 1); 


    tokens.erase(tokens.begin());
    // TODO: temporarily read in a smaller embedding instead of 768-dim
    tokens.erase(tokens.begin() + EMBEDDING_DIMENSION, tokens.end());
    
    EntityEmbedding record(id, tokens);
    EntityEmbeddingTable->BufferedAsyncInsert(handle, record.key(), record);
    count++;
  }
#ifdef DEBUG
  std::cout << this_locale << " read " << count << " lines" << std::endl;
#endif
  file.close();
}

void readRelationEmbeddingFile(Handle & handle, const RF_args_t & args) {
  std::string line;
  struct stat stats;
  std::string filename = args.relation_embedding_filename;
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  uint64_t num_locales = (uint64_t) shad::rt::numLocalities();

  std::ifstream file(filename);
  if (! file.is_open()) { 
    printf("Locale %lu cannot open file %s\n", this_locale, filename.c_str()); exit(-1); 
  }

  stat(filename.c_str(), & stats);

  uint64_t num_bytes = stats.st_size / num_locales;             // file size / number of locales
  uint64_t start = this_locale * num_bytes;
  uint64_t end = start + num_bytes;

  if (this_locale != 0) {                                       // check for partial line
    file.seekg(start - 1);
    getline(file, line);
    if (line[0] != '\n') start += line.size();                 // if not at start of a line, discard partial line
  }

  if (this_locale == num_locales - 1) end = stats.st_size;      // last locale processes to end of file


  auto count = 0;

  auto RelationEmbeddingTable = RelationEmbeddingType::GetPtr( (RelationEmbeddingOID) 
    						      args.relation_embedding_table_OID);

  while (start < end) {
    getline(file, line);
    start += line.size() + 1;
    if (count == 0 && this_locale == 0) {count++; continue; /*skip first line*/}

    auto id = std::stoull (line.substr(0, line.find_first_of(",")));
    std::vector <float> tokens = split_embedding(line, ',', EMBEDDING_DIM + 1);     // delimiter and # tokens set for wikidata data file


    tokens.erase(tokens.begin());
    tokens.erase(tokens.begin() + EMBEDDING_DIMENSION, tokens.end());

    RelationEmbedding record(id, tokens);
    RelationEmbeddingTable->BufferedAsyncInsert(handle, record.key(), record);

    count++;
  }
  file.close();
}
} // namespace agile::wk_multihop

