#ifndef _DRC643_HH_
#define _DRC643_HH_

#include "Computation.hh"
#include "ECBase.hh"
#include "ECDAG.hh"

#define DRC_MAX 32

using namespace std;

class DRC643 : public ECBase {
  private:
    int _m;
    int _r;
    int _nr;
    int _chunk_num_per_node;
    int _sys_chunk_num;
    int _enc_chunk_num;
    int _total_chunk_num;
    int _enc_matrix[DRC_MAX*DRC_MAX];
    bool _locality;

    void generate_encoding_matrix();

  public:
    DRC643(int n, int k, int cps, int opt, vector<string> param);
    ECDAG* Encode();
    ECDAG* Decode(vector<int> from, vector<int> to);
    void Place(vector<vector<int>>& group);
};

#endif
