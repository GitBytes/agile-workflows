#include <limits>
#include <sys/stat.h>

#include "agile/workflow3/main.h"
#include "agile/workflow3/graph.h"

namespace agile::workflow3 {

void readFASTA(Handle & handle, const Args_t & args) {
  std::string filename = args.filename;
  auto KMap = KMapType::GetPtr((KMapOID) args.KMap_OID);
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  uint64_t num_locales = (uint64_t) shad::rt::numLocalities();

  uint64_t one = 1;
  std::string line;
  struct stat stats;
  std::ifstream file(filename);
  if (! file.is_open()) { printf("Locale %lu cannot open file %s\n", this_locale, filename.c_str()); exit(-1); }

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

  while (start < end) {
    getline(file, line);
    start += line.size() + 1;
    if (line[0] == '>') continue;                                        // skip comments

    uint64_t kmer = 0;
    for (uint64_t i = 0; i < args.mnLength; ++ i)
      kmer = (kmer << 2) + CHAR_TO_EL(line[i]);                          // ... shift 2 bits left and add next char

    for (uint64_t i = args.mnLength; i < line.size(); ++ i) {            // for each char until end of line
      kmer = (kmer << 2) + CHAR_TO_EL(line[i]);                          // ... shift 2 bits left and add next char
      KMap->BufferedAsyncInsert(handle, kmer, one);
  } }

  file.close();
}

} // namespace agile::workflow3
