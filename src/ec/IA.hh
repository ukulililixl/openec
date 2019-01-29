#ifndef _IA_HH_
#define _IA_HH_d

#include "Computation.hh"
#include "ECBase.hh"
#include "ECDAG.hh"

#define IA_MAX 32

using namespace std;

class IA : public ECBase {
  private:
    int _chunk_num_per_node;
    int _sys_chunk_num;
    int _enc_chunk_num;
    int _total_chunk_num;

    int _ori_encoding_matrix[IA_MAX*IA_MAX];
    int _dual_enc_matrix[IA_MAX*IA_MAX];
    int _offline_enc_vec[IA_MAX*IA_MAX];
    int _final_enc_matrix[IA_MAX*IA_MAX];
    int _recovery_equations[IA_MAX*IA_MAX];

    void generate_encoding_matrix();
    void generate_decoding_matrix(int rBlkIdx);
    void square_cauchy_matrix(int *des, int size);
  public:
    IA(int n, int k, int w, int opt, vector<string> param);
    ECDAG* Encode();
    ECDAG* Decode(vector<int> from, vector<int> to);
    void Place(vector<vector<int>>& group);
};


#endif
