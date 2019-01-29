#ifndef _BUTTERFLY64_HH_
#define _BUTTERFLY64_HH

#include "Computation.hh"
#include "ECBase.hh"
#include "ECDAG.hh"

#define BUTTERFLY_MAX 64

using namespace std;

class BUTTERFLY64: public ECBase {
  private:
    int _m;
    int _r;
    int _nr;
    int _chunk_num_per_node;
    int _sys_chunk_num;
    int _enc_chunk_num;
    int _total_chunk_num;
    int _enc_matrix[BUTTERFLY_MAX*BUTTERFLY_MAX];
    int _tmp;

    void generate_encoding_matrix();
    void dump_matrix(int* matrix, int row, int col);
  public:
    BUTTERFLY64(int n, int k, int cps, int opt, vector<string> param);
    ECDAG* Encode();
    ECDAG* Decode(vector<int> from, vector<int> to);
    void Place(vector<vector<int>>& group);
};


#endif
