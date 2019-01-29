#ifndef _DRC963_HH_
#define _DRC963_HH_

#include "Computation.hh"
#include "ECBase.hh"
#include "ECDAG.hh"

using namespace std;

#define DRC963_MAX 64

class DRC963 : public ECBase {
  private:
    int _m;
    int _r;
    int _nr;
    int _chunk_num_per_node;
    int _sys_chunk_num;
    int _enc_chunk_num;
    int _total_chunk_num;
    int _enc_matrix[DRC963_MAX * DRC963_MAX];
    bool _locality;

    void generate_encoding_matrix();
  public:
    DRC963(int n, int k, int w, int opt, vector<string> param);
    ECDAG* Encode();
    ECDAG* Decode(vector<int> from, vector<int> to);
    void Place(vector<vector<int>>& group);
};

#endif
