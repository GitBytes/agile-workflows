#include "main.h"

namespace shad {
  using namespace agile::kernel_constructTables;

std::vector <std::string> split(std::string & line, char delim, uint64_t size = 0) {
  uint64_t ndx = 0, start = 0;
  std::vector <std::string> tokens(size);

  for (uint64_t end = 0; end < line.length(); end ++) {

    if ( (line[end] == delim) || (line[end] == '\n') ) {
       tokens[ndx] = line.substr(start, end - start);
       start = end + 1;
       ndx ++;
  } }

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

  auto Persons      = PersonVertexType::GetPtr     ((PersonVertexOID)      args.Persons_OID);
  auto ForumEvents  = ForumEventVertexType::GetPtr ((ForumEventVertexOID)  args.ForumEvents_OID);
  auto Forums       = ForumVertexType::GetPtr      ((ForumVertexOID)       args.Forums_OID);
  auto Publications = PublicationVertexType::GetPtr((PublicationVertexOID) args.Publications_OID);
  auto Topics       = TopicVertexType::GetPtr      ((TopicVertexOID)       args.Topics_OID);
  auto Purchases    = PurchaseEdgeType::GetPtr     ((PurchaseEdgeOID)      args.Purchases_OID);
  auto Sales        = SaleEdgeType::GetPtr         ((SaleEdgeOID)          args.Sales_OID);
  auto Authors      = AuthorEdgeType::GetPtr       ((AuthorEdgeOID)        args.Authors_OID);
  auto Includes     = IncludesEdgeType::GetPtr     ((IncludesEdgeOID)      args.Includes_OID);
  auto HasTopic     = HasTopicEdgeType::GetPtr     ((HasTopicEdgeOID)      args.HasTopic_OID);
  auto HasOrg       = HasOrgEdgeType::GetPtr       ( (HasOrgEdgeOID)        args.HasOrg_OID);

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
} } }


int main(int argc, char *argv[]) {
  Handle handle;
  double time1 = my_timer();
  std::string dataFile = argv[1];

  auto Persons      = PersonVertexType::Create(MEDIUM);
  auto ForumEvents  = ForumEventVertexType::Create(MEDIUM);
  auto Forums       = ForumVertexType::Create(SMALL);
  auto Publications = PublicationVertexType::Create(SMALL);
  auto Topics       = TopicVertexType::Create(SMALL);
  auto Purchases    = PurchaseEdgeType::Create(MEDIUM);
  auto Sales        = SaleEdgeType::Create(MEDIUM);
  auto Authors      = AuthorEdgeType::Create(LARGE);
  auto Includes     = IncludesEdgeType::Create(LARGE);
  auto HasTopic     = HasTopicEdgeType::Create(LARGE);
  auto HasOrg       = HasOrgEdgeType::Create(MEDIUM);

  RF_args_t args;
  args.Persons_OID      = (uint64_t) (Persons->GetGlobalID());
  args.ForumEvents_OID  = (uint64_t) (ForumEvents->GetGlobalID());
  args.Forums_OID       = (uint64_t) (Forums->GetGlobalID());
  args.Publications_OID = (uint64_t) (Publications->GetGlobalID());
  args.Topics_OID       = (uint64_t) (Topics->GetGlobalID());
  args.Purchases_OID    = (uint64_t) (Purchases->GetGlobalID());
  args.Sales_OID        = (uint64_t) (Sales->GetGlobalID());
  args.Authors_OID      = (uint64_t) (Authors->GetGlobalID());
  args.Includes_OID     = (uint64_t) (Includes->GetGlobalID());
  args.HasTopic_OID     = (uint64_t) (HasTopic->GetGlobalID());
  args.HasOrg_OID       = (uint64_t) (HasOrg->GetGlobalID());
  memcpy(args.filename, dataFile.c_str(), dataFile.size() + 1);

  shad::rt::asyncExecuteOnAll(handle, readFile, args);
  waitForCompletion(handle);

  Persons->WaitForBufferedInsert();
  ForumEvents->WaitForBufferedInsert();
  Forums->WaitForBufferedInsert();
  Publications->WaitForBufferedInsert();
  Topics->WaitForBufferedInsert();
  Purchases->WaitForBufferedInsert();
  Sales->WaitForBufferedInsert();
  Authors->WaitForBufferedInsert();
  Includes->WaitForBufferedInsert();
  HasTopic->WaitForBufferedInsert();
  HasOrg->WaitForBufferedInsert();

  printf("Time for graph construction = %lf\n\n", my_timer() - time1);

  printf("Number of persons      = %lu\n", Persons->Size());
  printf("Number of forum_events = %lu\n", ForumEvents->Size());
  printf("Number of forums       = %lu\n", Forums->Size());
  printf("Number of publications = %lu\n", Publications->Size());
  printf("Number of topics       = %lu\n", Topics->Size());

  printf("\n");
  printf("Number of purchase edges = %lu\n", Purchases->Size());
  printf("Number of sale edges     = %lu\n", Sales->Size());
  printf("Number of author edges   = %lu\n", Authors->Size());
  printf("Number of include edges  = %lu\n", Includes->Size());
  printf("Number of hasTopic edges = %lu\n", HasTopic->Size());
  printf("Number of hasOrg edges   = %lu\n", HasOrg->Size());

  printf("\n");
  printf("Total number of vertices = %lu\n",
       Persons->Size() + ForumEvents->Size() + Forums->Size() + Publications->Size() + Topics->Size());
  printf("Total number of edges    = %lu\n", 
       Purchases->Size() + Sales->Size() + Authors->Size() + Includes->Size() + HasTopic->Size() + HasOrg->Size());

/***** write out data structures for downstream kernels *****/
  printf("{Persons:\n");
  printf("{Forum Events:\n");
  printf("{Forums:\n");
  printf("{Publications:\n");
  printf("{Topics:\n");
  printf("{Purchases:\n");
  printf("{Sales:\n");
  printf("{Authors:\n");
  printf("{Includes:\n");
  printf("{Has Topic:\n");
  printf("{Has Org:\n");

  return 0;
}

}
