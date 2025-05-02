#include <algorithm>
#include <queue>
#include <limits>

#include "agile/wk_multihop/main.h"
#include "agile/wk_multihop/graph.h"

#define TURING_AWARD 11020773
#define DEEP_LEARNING 12090508
#define TOP_K 50

namespace agile::wk_multihop {

void computeScorePerLocale(Handle & handle, const 
			   std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, 
			   uint64_t, uint64_t> & args) {
  Handle innerHandle;
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  auto EntityEmbeddingTable = EntityEmbeddingType::GetPtr( (EntityEmbeddingOID) 
							   std::get<1>(args));


  auto RelationEmbeddingTable = RelationEmbeddingType::GetPtr( (RelationEmbeddingOID) 
  							       std::get<2>(args));
  auto PersonTable = PersonVertexType::GetPtr( (PersonVertexOID) std::get<0>(args));

  auto EntityScoreTable = EntityScoreType::GetPtr( (EntityScoreOID) 
							   std::get<5>(args));


  EntityEmbeddingType::LookupResult head_embedding;
  EntityEmbeddingTable->AsyncLookup(innerHandle, std::get<3>(args), & head_embedding);

  waitForCompletion(innerHandle);
  auto head_emb = head_embedding.value;

  RelationEmbeddingType::LookupResult relation_embedding;
  RelationEmbeddingTable->AsyncLookup(innerHandle, std::get<4>(args),
  				      & relation_embedding);

  waitForCompletion(innerHandle);
  auto relation_emb = relation_embedding.value;

  std::vector<std::tuple<uint64_t, float> > scores;
  auto gamma = 1.0;
  auto LocalPersonTable = PersonVertexType::GetPtr( (PersonVertexOID) std::get<0>(args))
    ->GetLocalHashmap();
  for (auto itr = LocalPersonTable->begin(); itr != LocalPersonTable->end(); ++itr) {
    auto person_id = (*itr).second.id_;
    EntityEmbeddingType::LookupResult person_embedding;
    EntityEmbeddingTable->AsyncLookup(innerHandle, person_id, & person_embedding);
    waitForCompletion(innerHandle);
    if (person_embedding.found) {
      auto person_emb = person_embedding.value;
      std::array<float, EMBEDDING_DIMENSION> score_arr;
      for (auto i = 0; i < EMBEDDING_DIMENSION; i++) {
	score_arr[i] = head_emb.embedding_[i] + relation_emb.embedding_[i] 
	  - person_emb.embedding_[i]; 
      }
      auto total_score = 0.0;
      for (auto i = 0; i < EMBEDDING_DIMENSION; i++) {
	total_score += std::fabs(score_arr[i]); // L_1 norm
      }
      auto TransE_score = gamma - total_score;
      EntityEmbeddingBasedScore record(this_locale, person_id, TransE_score);
      EntityScoreTable->BufferedAsyncInsert(handle, record.key(), record);
    } else {
      EntityEmbeddingBasedScore record(this_locale, person_id, 
				       std::numeric_limits<float>::lowest());
      EntityScoreTable->BufferedAsyncInsert(handle, record.key(), record);
    }
  }
}


void computeScoreBasedOnWorksInDL(Handle & handle, const 
				  std::tuple<uint64_t, uint64_t, uint64_t, 
				  uint64_t, uint64_t, uint64_t, uint64_t> & args) {
  Handle innerHandle;
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  auto EntityEmbeddingTable = EntityEmbeddingType::GetPtr( (EntityEmbeddingOID) 
							   std::get<1>(args));
  auto RelationEmbeddingTable = RelationEmbeddingType::GetPtr( (RelationEmbeddingOID) 
  							       std::get<2>(args));
  auto PersonTable = PersonVertexType::GetPtr( (PersonVertexOID) std::get<0>(args));

  auto EntityScoreTable = EntityScoreType::GetPtr( (EntityScoreOID) 
							   std::get<5>(args));
  // TODO: Fetch the following two only once before spawn
  EntityEmbeddingType::LookupResult head_embedding;
  EntityEmbeddingTable->AsyncLookup(innerHandle, std::get<3>(args), & head_embedding);

  waitForCompletion(innerHandle);

  RelationEmbeddingType::LookupResult relation_embedding;
  RelationEmbeddingTable->AsyncLookup(innerHandle, std::get<4>(args),
  				      & relation_embedding);

  waitForCompletion(innerHandle);
  auto head_emb = head_embedding.value;
  auto relation_emb = relation_embedding.value;

  std::vector<std::tuple<uint64_t, float> > scores;
  auto gamma = 1.0;

  auto person_id = std::get<6>(args);
  
  EntityEmbeddingType::LookupResult person_embedding;
  EntityEmbeddingTable->AsyncLookup(innerHandle, person_id, & person_embedding);
  waitForCompletion(innerHandle);
  
  if (person_embedding.found) {
    auto person_emb = person_embedding.value;
    std::array<float, EMBEDDING_DIMENSION> score_arr;
    for (auto i = 0; i < EMBEDDING_DIMENSION; i++) {
      score_arr[i] = head_emb.embedding_[i] + relation_emb.embedding_[i] 
	- person_emb.embedding_[i]; 
    }
    auto total_score = 0.0;
    for (auto i = 0; i < EMBEDDING_DIMENSION; i++) {
      total_score += std::fabs(score_arr[i]); // L_1 norm
    }
    auto TransE_score = gamma - total_score;
    EntityEmbeddingBasedScore record(this_locale, person_id, TransE_score);
    EntityScoreTable->BufferedAsyncInsert(handle, record.key(), record);
  } else {
    EntityEmbeddingBasedScore record(this_locale, person_id, 
				     std::numeric_limits<float>::lowest());
    EntityScoreTable->BufferedAsyncInsert(handle, record.key(), record);
  }
}


void computeAffiliationScorePerLocal(Handle & handle, const 
	  std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t> & args) {
  // get localhashmap pointer for University/institution
  // for each of the university, compute score
  // insert them in the multimap as (locality, (uni id, score))
  Handle innerHandle;
  uint64_t this_locale = (uint32_t) shad::rt::thisLocality();
  auto EntityEmbeddingTable = EntityEmbeddingType::GetPtr( (EntityEmbeddingOID) 
							   std::get<1>(args));
  auto RelationEmbeddingTable = RelationEmbeddingType::GetPtr( (RelationEmbeddingOID) 
  							       std::get<2>(args));
  auto UniversityTable = UniversityVertexType::GetPtr( (UniversityVertexOID) 
						       std::get<0>(args));

  auto EntityScoreTable = EntityScoreType::GetPtr( (EntityScoreOID) 
						   std::get<5>(args));

  EntityEmbeddingType::LookupResult head_embedding;
  EntityEmbeddingTable->AsyncLookup(innerHandle, std::get<3>(args), & head_embedding);
  waitForCompletion(innerHandle);

  RelationEmbeddingType::LookupResult relation_embedding;
  RelationEmbeddingTable->AsyncLookup(innerHandle, std::get<4>(args),
  				      & relation_embedding);

  waitForCompletion(innerHandle);
  auto head_emb = head_embedding.value;
  auto relation_emb = relation_embedding.value;

  std::vector<std::tuple<uint64_t, float> > scores;
  auto gamma = 1.0;
  auto LocalUniversityTable = UniversityVertexType::GetPtr( (UniversityVertexOID) 
						std::get<0>(args))->GetLocalHashmap();
  for (auto itr = LocalUniversityTable->begin(); itr != LocalUniversityTable->end(); ++itr) {
    auto university_id = (*itr).second.id_;
    EntityEmbeddingType::LookupResult university_embedding;
    EntityEmbeddingTable->AsyncLookup(innerHandle, university_id, & university_embedding);
    waitForCompletion(innerHandle);
    if (university_embedding.found) {
      auto university_emb = university_embedding.value;
      std::array<float, EMBEDDING_DIMENSION> score_arr;
      for (auto i = 0; i < EMBEDDING_DIMENSION; i++) {
	score_arr[i] = head_emb.embedding_[i] + relation_emb.embedding_[i] 
	  - university_emb.embedding_[i]; 
      }
      auto total_score = 0.0;
      for (auto i = 0; i < EMBEDDING_DIMENSION; i++) {
	total_score += std::fabs(score_arr[i]); // L_1 norm
      }
      auto TransE_score = gamma - total_score;
      EntityEmbeddingBasedScore record(this_locale, university_id, TransE_score);
      EntityScoreTable->BufferedAsyncInsert(handle, record.key(), record);
    } else {
      EntityEmbeddingBasedScore record(this_locale, university_id, 
				       std::numeric_limits<float>::lowest());
      EntityScoreTable->BufferedAsyncInsert(handle, record.key(), record);
    }
  }
}

void ComputeTopK(uint64_t EntityScoreTableGID,
				  std::vector<uint64_t> & current_top_k_list) {
  uint64_t num_locales = (uint64_t) shad::rt::numLocalities();
  auto EntityScoreTable = EntityScoreType::GetPtr( (EntityScoreOID) 
						   EntityScoreTableGID);
  std::vector<EntityScoreInfo> merged_top_k_list;
  current_top_k_list.clear();
  std::vector<float> top_k_scores;
  /*From each of the locale, retrieve the score vector, insert locale-specific 
    top k element in merged list*/
  for (auto i = 0; i < num_locales; i++) {
    std::priority_queue<EntityScoreInfo, std::vector<EntityScoreInfo>,
			EntityScoreComparatorLess> top_k_elements;
    EntityScoreType::LookupResult res;
    EntityScoreTable->Lookup(i, & res);
    if (res.size == 0) {continue;}
    for (auto & e : res.value) {
      EntityScoreInfo record(e.score_, e.id_);
      top_k_elements.push(record);
    }

    for (auto i = 0; i < TOP_K; i++) {
      auto item = top_k_elements.top();
      merged_top_k_list.push_back(item);
      top_k_elements.pop();
    }
  }
  EntityScoreComparator entityScoreComparator;
  std::sort(merged_top_k_list.begin(), merged_top_k_list.end(), entityScoreComparator);
  // TODO: enable C++17 execution policy for parallel sort

  for (auto i = 0; i < TOP_K; i++) {
    current_top_k_list.push_back(merged_top_k_list[i].id_);
#ifdef DEBUG
    std::cout << "ID: " << merged_top_k_list[i].id_ << " score " << merged_top_k_list[i].score_ 
	      <<std::endl;
#endif
  }
}


void multiHopReasoning(uint64_t entity_embedding_table_oid, 
		       uint64_t relation_embedding_table_oid,
		       uint64_t person_table_oid,
		       uint64_t university_table_oid) {
  Handle handle;

  uint64_t num_locales = (uint64_t) shad::rt::numLocalities();
  auto this_locale = (uint32_t) shad::rt::thisLocality();
  std::vector<uint64_t> current_top_k_list;
  std::vector<uint64_t> top_k_affiliation_list;

  auto EntityScoreTable = EntityScoreType::Create(AGILE_LARGE);
  auto EntityScoreTableGID = (uint64_t) (EntityScoreTable->GetGlobalID());
  
  auto args = std::make_tuple(person_table_oid, entity_embedding_table_oid,
			      relation_embedding_table_oid, (uint64_t) TURING_AWARD,
			      (uint64_t) AWARD_WINNER_TABLE_IDX,
			      EntityScoreTableGID);
  shad::rt::asyncExecuteOnAll(handle, computeScorePerLocale, args);
  shad::rt::waitForCompletion(handle);

  EntityScoreTable->WaitForBufferedInsert();

  // auto percent_select = 0.1;
  ComputeTopK(EntityScoreTableGID, current_top_k_list); //, percent_select);

  std::cout << "Top k potential Turing Award winners: " << std::endl;
  for (auto person : current_top_k_list) {
    std::cout << person << " ";
  }
  std::cout << std::endl;

  /*Now consider deep learning and potential set of award winners who work in this field*/
  EntityScoreTable->Clear();

  for (auto person_id : current_top_k_list) {
    auto  args_ = std::make_tuple(person_table_oid, entity_embedding_table_oid,
			   relation_embedding_table_oid, (uint64_t) DEEP_LEARNING,
			   (uint64_t) WORKS_IN_TABLE_IDX,
			   EntityScoreTableGID, person_id);

    shad::rt::asyncExecuteAt(handle, shad::rt::thisLocality(), 
			     computeScoreBasedOnWorksInDL, args_);
  }

  waitForCompletion(handle);
  EntityScoreTable->WaitForBufferedInsert();

  ComputeTopK(EntityScoreTableGID, current_top_k_list);

  std::cout << "Top k potential Turing Award winners working in the DL field: " << std::endl;
  for (auto person : current_top_k_list) {
    std::cout << person << " ";
  }
  std::cout << std::endl;


  /*Find affiliations/University*/
  for (auto person_id : current_top_k_list) {
    EntityScoreTable->Clear();
    auto  args_ = std::make_tuple(university_table_oid, entity_embedding_table_oid,
			   relation_embedding_table_oid, person_id,
			   (uint64_t) AFFILIATED_WITH_TABLE_IDX,
			   EntityScoreTableGID);

    shad::rt::asyncExecuteOnAll(handle, computeAffiliationScorePerLocal, args_);
    waitForCompletion(handle);
    EntityScoreTable->WaitForBufferedInsert();

    ComputeTopK(EntityScoreTableGID, top_k_affiliation_list);

    std::cout << "Person " << person_id << " 's topmost potential affiliation " 
	      << top_k_affiliation_list[0] << std::endl;
  }
}

#ifdef ENABLE_TEST
void Find_Turing_Award_Winners(Handle & handle, const uint64_t & src, 
			 std::vector <WikiDataEdge> & award_winners) {
  // 575190993,85367025,207,11020773 # hinton turing award
  auto WorksInEdgeTable = WikiDataEdgeType::GetPtr( (WikiDataEdgeOID) 
						    graph[WORKS_IN_TABLE_IDX]); 
  auto AffiliatedWithEdgeTable = WikiDataEdgeType::GetPtr( (WikiDataEdgeOID) 
							   graph[AFFILIATED_WITH_TABLE_IDX]);

  for (auto award_winner : award_winners) {
    if (award_winner.dst_v == TURING_AWARD) {
      // 575190998,85367025,3,12090508 hinton, deeplearning
      WikiDataEdgeType::LookupResult res;
      auto is_working_in_dl = WorksInEdgeTable->Lookup(award_winner.src_v, & res);
      if (res.size == 0) {continue;}
      else {
	for (auto & working_in : res.value) {
	  if (working_in.dst_v == DEEP_LEARNING) {
	    // 575190992,85367025,40,10601699 hinton, UoT
	    WikiDataEdgeType::LookupResult affiliations;
	    AffiliatedWithEdgeTable->Lookup(award_winner.src_v, & affiliations);
	    for (auto & affiliation : affiliations.value) {
	      std::cout << award_winner.src_v << " is affiliated with " 
			<< affiliation.dst_v <<  std::endl;
}}}}}}}

void WikiData_pattern() {
  Handle handle;

  auto AwardWinnerEdgeTable = WikiDataEdgeType::GetPtr( (WikiDataEdgeOID) 
						  graph[AWARD_WINNER_TABLE_IDX]); 
  AwardWinnerEdgeTable->AsyncForEachEntry(handle, Find_Turing_Award_Winners);
  
  waitForCompletion(handle);
}
#endif

} // namespace agile::wk_multihop

