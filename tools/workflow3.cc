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
//
#include "agile/workflow3/main.h"
#include "agile/workflow3/graph.h"

namespace shad {
  using namespace agile::workflow3;

int main(int argc, char *argv[]) {
  if (argc != 6) {
     printf("Command parameters: <file name> <kmer_length> ");
     printf("<coverage> <min counts> <node threshold> \n");
     exit(-1);
  }

  double time1 = my_timer();
  std::string filename = argv[1];
  uint64_t min_counts  = std::stoull(argv[4]);

//********** CREATE DATA STRUCTURES AND ARGS **********//
  rt:: Handle handle;
  auto KMap = KMapType::Create(LARGE);                     // distinct kmer hashmap
  auto KVMap = KMapType::Create(LARGE);                    // valid kmer hashmap
  auto MNMap = MNMapType::Create(LARGE);                   // macro node multimap
  auto WireMap = WireMapType::Create(LARGE);               // wire multimap

  auto bucketCounts = IntArray::Create(min_counts, 0);     // array to count kmers appearing [1..min_count] times
  bucketCounts->FillPtrs();

  Args_t args;
  args.KMap_OID         = (uint64_t) (KMap->GetGlobalID());
  args.KVMap_OID        = (uint64_t) (KVMap->GetGlobalID());
  args.MNMap_OID        = (uint64_t) (MNMap->GetGlobalID());
  args.WireMap_OID      = (uint64_t) (WireMap->GetGlobalID());
  args.bucketCounts_OID = (uint64_t) (bucketCounts->GetGlobalID());

  args.kmer_length    = std::stoull(argv[2]);
  args.coverage       = std::stoull(argv[3]);
  args.min_counts     = std::stoull(argv[4]);
  args.node_threshold = std::stoull(argv[5]);
  memcpy(args.filename, filename.c_str(), filename.size() + 1);

//********** READ FASTA FILE AND CONSTRUCT KMER HASH MAP **********//
  shad::rt::asyncExecuteOnAll(handle, readFASTA, args);                  // read FASTA file
  rt::waitForCompletion(handle);
  KMap->WaitForBufferedInsert();

  printf("Time to read FASTA file = %lf\n", my_timer() - time1);
  printf("Number of distinct k-mer entries = %lu\n", KMap->Size());

//********** CONSTRUCT MACRO NODES **********//
  time1 = my_timer();

  shad::rt::asyncExecuteOnAll(handle, BucketCounts, args);     // count number kmers appearing [1..min_count] times 
  rt::waitForCompletion(handle);

  args.min_index = 0;
  uint64_t min_count = ULLONG_MAX;

  for (uint64_t i = 1; i < min_counts; ++ i) {                 // compute ndx with minimum number of appearances
    uint64_t count = bucketCounts->At(i);
    if (count < min_count) {args.min_index = i; min_count = count;}
  }

  shad::rt::asyncExecuteOnAll(handle, RemoveKmers, args);      // move kmers that appear > min_index to KVMap
  rt::waitForCompletion(handle);
  KVMap->WaitForBufferedInsert();

  KMap->Clear();
  printf("Time to remove kmers = %lf\n", my_timer() - time1);
  printf("Kmers appearing less than %lu times have been removed\n", args.min_index);
  printf("Number of valid k-mer entries = %lu\n", KVMap->Size());

  time1 = my_timer();

  KVMap->AsyncForEachEntry(handle, ConstructMacroNodes, args);     // construct macro nodes
  rt::waitForCompletion(handle);
  MNMap->WaitForBufferedInsert();

  MNMap->AsyncForEachEntry(handle, InitialMacroNodeWire, args);     // initialize wiring
  rt::waitForCompletion(handle);
  WireMap->WaitForBufferedInsert();

  printf("Time to construct and wire macro nodes = %lf\n", my_timer() - time1);
  printf("Number of macro nodes = %lu\n", MNMap->Size());     // TODO: change to MNMap->NumberKeys()

// temp vector for storing all partial contigs generated during Phase 2
  // args.node_threshold = node_threshold;
  // std::vector<BasePairVector> partial_contig_list;
  // size_t global_num_nodes = begin_iterative_compaction(MN_map, partial_contig_list);

//retain a list of terminal prefixes for each individual process, potential begin k-mers
  // std::vector<BeginMN> list_of_begin_kmers;
  // identify_begin_kmers (MN_map, list_of_begin_kmers);

/* Perform an Allgather such that all macro_nodes are accessible to all procs */
  // std::vector<std::pair<kmer_t,MacroNode>> global_MN_map(global_num_nodes); // new map for storing all the macro_nodes
  // generate_compacted_pakgraph(MN_map, global_MN_map);
  // traverse_pakgraph(global_MN_map, list_of_begin_kmers, partial_contig_list);
  //

  return 0;
}

}
