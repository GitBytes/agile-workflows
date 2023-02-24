//===------------------------------------------------------------*- C++ -*-===//
////
////                            The AGILE Workflows
////
////===----------------------------------------------------------------------===//
//// ** Pre-Copyright Notice
////
//// This computer software was prepared by Battelle Memorial Institute,
//// hereinafter the Contractor, under Contract No. DE-AC05-76RL01830 with the
//// Department of Energy (DOE). All rights in the computer software are reserved
//// by DOE on behalf of the United States Government and the Contractor as
//// provided in the Contract. You are authorized to use this computer software
//// for Governmental purposes but it is not to be released or distributed to the
//// public. NEITHER THE GOVERNMENT NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS
//// OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. This
//// notice including this sentence must appear on any copies of this computer
//// software.
////
//// ** Disclaimer Notice
////
//// This material was prepared as an account of work sponsored by an agency of
//// the United States Government. Neither the United States Government nor the
//// United States Department of Energy, nor Battelle, nor any of their employees,
//// nor any jurisdiction or organization that has cooperated in the development
//// of these materials, makes any warranty, express or implied, or assumes any
//// legal liability or responsibility for the accuracy, completeness, or
//// usefulness or any information, apparatus, product, software, or process
//// disclosed, or represents that its use would not infringe privately owned
//// rights. Reference herein to any specific commercial product, process, or
//// service by trade name, trademark, manufacturer, or otherwise does not
//// necessarily constitute or imply its endorsement, recommendation, or favoring
//// by the United States Government or any agency thereof, or Battelle Memorial
//// Institute. The views and opinions of authors expressed herein do not
//// necessarily state or reflect those of the United States Government or any
//// agency thereof.
////
////                    PACIFIC NORTHWEST NATIONAL LABORATORY
////                                 operated by
////                                   BATTELLE
////                                   for the
////                      UNITED STATES DEPARTMENT OF ENERGY
////                       under Contract DE-AC05-76RL01830
////===----------------------------------------------------------------------===//

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
