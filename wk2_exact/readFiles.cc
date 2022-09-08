#include <limits>
#include <string>
#include <sys/stat.h>

#include "agile/wk2_exact/main.h"
#include "agile/wk2_exact/graph.h"

namespace agile::wk2_exact {

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

  auto Persons      = PersonVertexType::GetPtr( (PersonVertexOID) args.Persons_OID);
  auto ForumEvents  = ForumEventVertexType::GetPtr( (ForumEventVertexOID) args.ForumEvents_OID);
  auto Forums       = ForumVertexType::GetPtr( (ForumVertexOID) args.Forums_OID);
  auto Publications = PublicationVertexType::GetPtr( (PublicationVertexOID) args.Publications_OID);
  auto Topics       = TopicVertexType::GetPtr( (TopicVertexOID) args.Topics_OID);
  auto Purchases    = PurchaseEdgeType::GetPtr( (PurchaseEdgeOID) args.Purchases_OID);
  auto Sales        = SaleEdgeType::GetPtr( (SaleEdgeOID) args.Sales_OID);
  auto Authors      = AuthorEdgeType::GetPtr( (AuthorEdgeOID) args.Authors_OID);
  auto Includes     = IncludesEdgeType::GetPtr( (IncludesEdgeOID) args.Includes_OID);
  auto HasTopic     = HasTopicEdgeType::GetPtr( (HasTopicEdgeOID) args.HasTopic_OID);
  auto HasOrg       = HasOrgEdgeType::GetPtr( (HasOrgEdgeOID) args.HasOrg_OID);

  while (start < end) {
    getline(file, line);
    start += line.size() + 1;
    if (line[0] == '#') continue;                                // skip comments
    std::vector <std::string> tokens = split(line, ',', 10);     // delimiter and # tokens set for wmd data file

    if (tokens[0] == "Person") {
         PersonVertex record(tokens);
         Persons->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "ForumEvent") {
         ForumEventVertex record(tokens);
         ForumEvents->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Forum") {
         ForumVertex record(tokens);
         Forums->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Publication") {
         PublicationVertex record(tokens);
         Publications->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Topic") {
         TopicVertex record(tokens);
         Topics->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Sale") {
         SaleEdge record(tokens);
         Sales->BufferedAsyncInsert(handle, record.key(), record);
         PurchaseEdge record1(tokens);
         Purchases->BufferedAsyncInsert(handle, record1.key(), record1);
    } else if (tokens[0] == "Author") {
         AuthorEdge record(tokens);
         Authors->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "Includes") {
         IncludesEdge record(tokens);
         Includes->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "HasTopic") {
         HasTopicEdge record(tokens);
         HasTopic->BufferedAsyncInsert(handle, record.key(), record);
    } else if (tokens[0] == "HasOrg") {
         HasOrgEdge record(tokens);
         HasOrg->BufferedAsyncInsert(handle, record.key(), record);
  } }

  file.close();
}

} // namespace agile::wk2_exact
