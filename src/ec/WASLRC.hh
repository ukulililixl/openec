#ifndef _WASLRC_HH_
#define _WASLRC_HH_

#include "Computation.hh"
#include "ECBase.hh"

using namespace std;

#define RS_N_MAX (32)

class WASLRC : public ECBase {
  private:
    int _l;
    int _r;
    int _encode_matrix[RS_N_MAX * RS_N_MAX];     

    void generate_matrix(int* matrix, int k, int l, int r, int w);
  public:
    WASLRC(int n, int k, int cps, int opt, vector<string> param);

    ECDAG* Encode();
    ECDAG* Decode(vector<int> from, vector<int> to);
    void Place(vector<vector<int>>& group);
};

#endif
