void generate_id_set (std::vector<std::pair<kmer_t,MacroNode>> &MN_map, std::vector<size_t> &id_list_nodes) {

  for (size_t it = 0; it < MN_map.size(); it++) {     // for each macro node
      kmer_t key = MN_map[it].first;
      kmer_t tmp_kmer = 0;
      kmer_t max_kmer = key;
      MacroNode &mn = MN_map[it].second;

      bool key_notin_idset = false;

      for (size_t i = 0; i < mn.prefixes.size(); i++) {     // for each prefix
        tmp_kmer = 0;

        if (! mn.prefixes_terminal[i]) { 
           int len_pref = mn.prefixes[i].size();

           if (len_pref >= MN_LENGTH) {
              tmp_kmer = mn.prefixes[i].extract(MN_LENGTH);
           } else if (len_pref > 0) {
              size_t remainder = MN_LENGTH - len_pref;
              tmp_kmer = mn.prefixes[i].extract(len_pref);
              kmer_t temp_ext = mn_extract_pred(key, remainder);
              tmp_kmer = ((tmp_kmer << (remainder*2)) | temp_ext);
        }  }
             
        if (tmp_kmer > key) {
           max_kmer = tmp_kmer;
           key_notin_idset=true;
           break;
      } }

      tmp_kmer = 0;

      if (! key_notin_idset) {

         for (size_t i = 0; i < mn.suffixes.size(); i++) {
           tmp_kmer = 0;

           if (!mn.suffixes_terminal[i]) {
              int len_suff = mn.suffixes[i].size();
              if (len_suff >= MN_LENGTH) {
                 tmp_kmer = mn.suffixes[i].extract_succ(MN_LENGTH);
              } else if (len_suff > 0) {
                 size_t remainder = MN_LENGTH - len_suff;
                 tmp_kmer = mn_extract_succ(key, remainder);
                 kmer_t temp_ext = mn.suffixes[i].extract(len_suff);
                 tmp_kmer = ((tmp_kmer << (len_suff*2)) | temp_ext);
           }  }

           if (tmp_kmer > key) {
              max_kmer=tmp_kmer;
              key_notin_idset=true;
              break;
      }  } }

      if (! key_notin_idset) id_list_nodes.push_back(it);
} } // end of for loop


void iterate_and_pack_mn (std::vector<size_t>& id_list_nodes,
                          std::vector< std::vector<TransferNode> >& mn_nodes_per_proc,
                          std::vector<std::pair<kmer_t,MacroNode>> &MN_map,
                          std::vector<BasePairVector> &local_contig_list) {
 
  int itr_p = 0, itr_s = 0;

  for (size_t i = 0; i < id_list_nodes.size(); i++) {
    itr_p = 0;
    itr_s = 0;

    kmer_t &del_key   = MN_map[id_list_nodes[i]].first;
    MacroNode &del_mn = MN_map[id_list_nodes[i]].second;

    for (int k = 0; k < del_mn.prefix_begin_info.size(); k++) {
      if (del_mn.prefix_begin_info[k].num_wires) {
         itr_p = k;

         MnodeInfo search_pred_param;
         if ((del_mn.prefixes[itr_p].size() > 0) && (!del_mn.prefixes_terminal[itr_p]))
            search_pred_param = retrieve_mn_pinfo(del_mn.prefixes[itr_p], del_mn.k_1_mer);

         for (int t = 0; t < del_mn.prefix_begin_info[k].num_wires; t++) {
           itr_s = del_mn.wiring_info[del_mn.prefix_begin_info[k].prefix_pos+t].suffix_id;
           int count = del_mn.wiring_info[del_mn.prefix_begin_info[k].prefix_pos+t].count;

           if (del_mn.prefixes_terminal[itr_p] && del_mn.suffixes_terminal[itr_s]) {
              BasePairVector partial_contig = del_mn.prefixes[itr_p];
              partial_contig.append(del_mn.k_1_mer);
              partial_contig.append(del_mn.suffixes[itr_s]);
              local_contig_list.push_back(partial_contig);

           } else {
              MnodeInfo search_succ_param;
              if ((del_mn.suffixes[itr_s].size() > 0) && (!del_mn.suffixes_terminal[itr_s]))
                 search_succ_param = retrieve_mn_sinfo(del_mn.suffixes[itr_s], del_mn.k_1_mer);

              std::array<bool, 2> self_loop_info = {false, false};
              check_for_self_loops(search_pred_param.search_mn,
                                   search_succ_param.search_mn, del_key, self_loop_info);

              if (! del_mn.prefixes_terminal[itr_p]) {
                 if (! self_loop_info[0]) {

                    //determine new value terminal
                    bool new_pnode_type = false;
                    if (del_mn.suffixes_terminal[itr_s])
                       new_pnode_type = true;
                    else {
                       if (self_loop_info[1]) new_pnode_type = true;
                       else                   new_pnode_type = false;
                    }

                    mn_nodes_per_proc[retrieve_proc_id(search_pred_param.search_mn)].push_back (
                         TransferNode{
                              search_pred_param.search_mn,
                              search_pred_param.search_ext,
                              del_mn.suffixes[itr_s],
                              std::make_pair ( std::min(del_mn.prefix_count[itr_p].first,
                                                        del_mn.suffix_count[itr_s].first),
                                               count
                                             ),
                              new_pnode_type,
                              (KdirType) P
                         }
                    );
              }  }

              if (! del_mn.suffixes_terminal[itr_s]) {
                 if (! self_loop_info[1]) {

                    //determine new value terminal
                    bool new_snode_type=false;
                    if (del_mn.prefixes_terminal[itr_p])
                       new_snode_type = true;
                    else {
                       if (self_loop_info[0]) new_snode_type = true;
                       else                   new_snode_type = false;
                    }
                    mn_nodes_per_proc[retrieve_proc_id(search_succ_param.search_mn)].push_back(
                         TransferNode {
                              search_succ_param.search_mn,
                               search_succ_param.search_ext,
                               del_mn.prefixes[itr_p],
                               std::make_pair( std::min(del_mn.prefix_count[itr_p].first,
                                               del_mn.suffix_count[itr_s].first),
                                               count
                                             ),
                              new_snode_type,
                              (KdirType) S
                         }
                    );
           }  }  }     // end of else

         }            // end of for loop across num_wires
      }               // end of if condition
    }                 // end of for loop for a node
  }                   // end of for for list of nodes in id_set
}


void check_for_self_loops (kmer_t search_pkmer, kmer_t search_skmer,
                           kmer_t node, std::array<bool, 2> & self_loop_info) {
    if (search_pkmer == node) self_loop_info[0] = true;
    if (search_skmer == node) self_loop_info[1] = true;
}


void serialize_and_transfer (std::vector< std::vector<TransferNode> > & mn_nodes_per_proc,
                             std::vector<std::pair<kmer_t,MacroNode>> & MN_map,
                             std::vector<size_t> &rewire_pos_list,
                             int num_itr) {


  /* Perform Alltoallv to update the nodes */
  std::string send_buffer;
  std::string tmp_buffer;

  /* send and recv buffers for obtaining the actual number of macro_nodes */
  std::vector<uint64_t> send_count_buf(size,0);
  std::vector<uint64_t> recv_count_buf(size,0);

  /* send and recv buffers for obtaining the serialized data */
  std::vector<uint64_t> scounts(size,0); //sending serialized data in bytes
  std::vector<uint64_t> rcounts (size,0);
  std::vector<uint64_t> rdisp (size,0);
  std::vector<int> scounts_dd(size,0);
  std::vector<int> rcounts_dd(size,0);
  std::vector<int> sdisp_dd(size,0);
  std::vector<int> rdisp_dd(size,0);

  std::string recv_buffer;
  char * recv_ptr = nullptr;

  std::ostringstream os_cnt(std::ios::binary | std::ios::out | std::ios::in);

  //serialize
  uint64_t ssize = 0,rsize = 0,r_mnodes = 0;
  uint64_t pad_width = 1000;

  for (int i = 0; i < size; i++) {
    os_cnt.str("");
    tmp_buffer.clear();
    send_count_buf[i] = mn_nodes_per_proc[i].size();

    for (size_t j = 0; j < mn_nodes_per_proc[i].size(); j++) serialize(os_cnt, mn_nodes_per_proc[i][j]);

    tmp_buffer = os_cnt.str();
    if (tmp_buffer.size() > 0) {
       size_t nonPaddedSize = tmp_buffer.size();
       size_t padded_size = ((nonPaddedSize / pad_width) + (nonPaddedSize % pad_width != 0)) * pad_width;

       tmp_buffer.resize(padded_size, 0);
    }

    scounts[i] = tmp_buffer.length();
    scounts_dd[i] = scounts[i]/pad_width;
 
    send_buffer.append(tmp_buffer);
  }

  // clear the buffers
  tmp_buffer.clear();
  for (int i = 0; i < size; i++) mn_nodes_per_proc[i].clear();
  mn_nodes_per_proc.clear();

  for (int t = 0; t < size; t++) ssize += scounts[t];

  double comm1 = MPI_Wtime ();

  //Sending the counts
  MPI_Alltoall(send_count_buf.data(), 1, MPI_UINT64_T, recv_count_buf.data(), 1, MPI_UINT64_T, MPI_COMM_WORLD);

  //Sending the serialized data buffer
  MPI_Alltoall(scounts.data(), 1, MPI_UINT64_T, rcounts.data(), 1, MPI_UINT64_T, MPI_COMM_WORLD);

  double comm2 = MPI_Wtime ();
  p2alltoall_time += (comm2 - comm1);

  /* r_mnodes denotes the number of MN entries to deserialize and modify per rank */
  for (int t = 0; t < size; t++) r_mnodes += recv_count_buf[t];
  for (int t = 0; t < size; t++) rsize += rcounts[t];

  for (int t = 0; t < size; t++) {
    sdisp_dd[t] = (t>0) ? (scounts_dd[t-1] + sdisp_dd[t-1]) : 0;
    rdisp[t] = (t>0) ? (rcounts[t-1] + rdisp[t-1]) : 0;
    rcounts_dd[t] = rcounts[t]/pad_width;
    rdisp_dd[t] = (t>0) ? (rcounts_dd[t-1] + rdisp_dd[t-1]) : 0;
  }

  recv_buffer.resize(rsize, 'F');
  recv_ptr=&recv_buffer[0];
  MPI_Datatype rowtype;

  //create contiguous derived data type
  MPI_Type_contiguous(pad_width, MPI_BYTE, &rowtype);
  MPI_Type_commit(&rowtype);
  MPI_Barrier(MPI_COMM_WORLD);

  double comm3 = MPI_Wtime ();
  int result = MPI_Alltoallv(send_buffer.c_str(), scounts_dd.data(), sdisp_dd.data(), rowtype,
                             recv_ptr, rcounts_dd.data(), rdisp_dd.data(), rowtype, MPI_COMM_WORLD);

  if (result != MPI_SUCCESS) {
     printf("rank: %d, MPI_Alltoallv in Phase 2 failed with return value: %d\n", rank, result);
     MPI_Finalize();
     exit(2);
  }

  double comm4 = MPI_Wtime ();
  p2alltoallv_time += (comm4 - comm3);

  // free datatype
  MPI_Type_free(&rowtype);

  os_cnt.str("");
  scounts.clear();
  scounts_dd.clear();
  sdisp_dd.clear();
  send_buffer.clear();
  send_count_buf.clear();
  rcounts_dd.clear();
  rdisp_dd.clear();
     
  std::istringstream is(std::ios::binary | std::ios::out | std::ios::in);
  uint64_t k = 0;

  for (int i = 0; i < size; i++) {
    is.rdbuf()->pubsetbuf(const_cast<char*>(recv_buffer.c_str()+rdisp[i]), rcounts[i]);
    uint64_t nnodes = recv_count_buf[i];

    while (nnodes) {
      TransferNode mi;
      deserialize(is, mi);

      kmer_t search_key = mi.search_mn;
      int pos = find_mnode_exists(search_key, MN_map);
      if (MN_map[pos].first != search_key) {
         printf("rank: %d, MN node key was not found in map at the time of pushing to: %d\n", rank, mi.direction);
         MPI_Finalize();
         exit(2);
      }

      if (mi.direction == P) push_to_pred(mi, pos, MN_map);
      else                   push_to_succ(mi, pos, MN_map);

      rewire_pos_list.push_back(pos); 
      k++;
         nnodes--;
  } }

  if (k != r_mnodes)
     fprintf(stderr, "Error!! for rank: %d, Not matching, k:%lu, r_mnodes:%lu, ssize:%lu, rsize:%lu\n", 
                                               rank, k, r_mnodes, ssize, rsize);

  MPI_Barrier(MPI_COMM_WORLD);

  recv_buffer.clear();
  rcounts.clear();
  rdisp.clear();
  recv_count_buf.clear();
     
#ifdef DESER_V1
     return mn_nodes_to_modify;
#endif
}


//void push_to_pred (std::vector<ModNodeInfo> &new_mnode, int pos, 
void push_to_pred (TransferNode &new_mnode, int pos, std::vector<std::pair<kmer_t,MacroNode>> &MN_map) {
  MacroNode &mn_found = MN_map[pos].second;
    
  BasePairVector search_ext = new_mnode.search_ext;
  BasePairVector new_pext = search_ext;

  new_pext.append(new_mnode.mn_ext);
  std::pair<int,int> new_kcount = new_mnode.mn_count;
  bool new_ntype = new_mnode.mn_terminal;

  std::vector<BasePairVector>::iterator viter =
        std::find(mn_found.suffixes.begin(), mn_found.suffixes.end(), BasePairVector(search_ext));

  if (viter == mn_found.suffixes.end()) {     // not found
     mn_found.suffixes.push_back(new_pext);
     mn_found.suffix_count.push_back(new_kcount);
     mn_found.suffixes_terminal.push_back(new_ntype);
  } else {                                    // found 
     int idx = std::distance(mn_found.suffixes.begin(), viter);
     if (mn_found.suffixes_terminal[idx]) {
        mn_found.suffixes.push_back(new_pext);
        mn_found.suffix_count.push_back(new_kcount);
        mn_found.suffixes_terminal.push_back(new_ntype);
     } else {
        mn_found.suffixes[idx]=new_pext;
        mn_found.suffix_count[idx]=new_kcount;
        mn_found.suffixes_terminal[idx]=new_ntype;
} }  }


void push_to_succ (TransferNode &new_mnode, int pos, std::vector<std::pair<kmer_t,MacroNode>> &MN_map) {
  MacroNode &mn_found = MN_map[pos].second;

  BasePairVector search_ext = new_mnode.search_ext;
  BasePairVector new_sext = new_mnode.mn_ext;

  new_sext.append(search_ext);
  std::pair<int,int> new_kcount = new_mnode.mn_count;
  bool new_ntype = new_mnode.mn_terminal;

  std::vector<BasePairVector>::iterator viter =
        std::find(mn_found.prefixes.begin(), mn_found.prefixes.end(), BasePairVector(search_ext));

  if (viter == mn_found.prefixes.end()) {     // not found
     mn_found.prefixes.push_back(new_sext);
     mn_found.prefix_count.push_back(new_kcount);
     mn_found.prefixes_terminal.push_back(new_ntype);
  } else {                                    // found 
     int idx = std::distance(mn_found.prefixes.begin(), viter);
     if (mn_found.prefixes_terminal[idx]) {
        mn_found.prefixes.push_back(new_sext);
        mn_found.prefix_count.push_back(new_kcount);
        mn_found.prefixes_terminal.push_back(new_ntype);
     } else {
        mn_found.prefixes[idx]=new_sext;
        mn_found.prefix_count[idx]=new_kcount;
        mn_found.prefixes_terminal[idx]=new_ntype;
} }  }


size_t begin_iterative_compaction (
    std::vector<std::pair<kmer_t,MacroNode>> &MN_map, std::vector<BasePairVector> &partial_contig_list) {

    int num_itr = 0;
    size_t num_nodes = 0, global_num_nodes = 0;
    std::vector<size_t> id_list_nodes;
    std::vector<size_t> rewire_pos_list;

    while (1) {
      num_itr ++;
      num_nodes = MNMap->NumberKeys();
      printf("Itr: %lu, Total number of macro nodes: %lu\n", num_itr, num_nodes);

      if (num_nodes <= node_threashold) return global_num_nodes;     // all done

      generate_id_set(MN_map, id_list_nodes);

      std::vector< std::vector<TransferNode> > mn_nodes_per_proc(size);
      iterate_and_pack_mn(id_list_nodes, mn_nodes_per_proc, MN_map, partial_contig_list);

      // TODO: remove id_list nodes from MNMap 
      __gnu_parallel::sort(MN_map.begin(), MN_map.end(), Comp_pair); 

      serialize_and_transfer(mn_nodes_per_proc, MN_map, rewire_pos_list, num_itr);

      std::sort(rewire_pos_list.begin(), rewire_pos_list.end());
      rewire_pos_list.erase( unique( rewire_pos_list.begin(), rewire_pos_list.end() ), rewire_pos_list.end() );

      for (size_t i = 0; i < rewire_pos_list.size(); i++) {
        MacroNode &mn = MN_map[rewire_pos_list[i]].second;

        mn.wiring_info.clear();
        mn.prefix_begin_info.clear();

        int num_p = mn.prefixes.size();
        int num_s = mn.suffixes.size();

        mn.wiring_info.resize(num_p+num_s + 1);
        mn.prefix_begin_info.resize(num_p);
        mn.setup_wiring();
      }

      rewire_pos_list.clear();
} }
