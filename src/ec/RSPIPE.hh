#ifndef _RSPIPE_HH_
#define _RSPIPE_HH_

#include "Computation.hh"
#include "ECBase.hh"
#include "ECDAG.hh"

#define RSPIPE_N_MAX 32

using namespace std;

class RSPIPE : public ECBase {
  private:
    int _m;
    int _encode_matrix[RSPIPE_N_MAX * RSPIPE_N_MAX];

    void generate_matrix(int* matrix, int rows, int cols, int w);
  public:
    RSPIPE(int n, int k, int w, int opt, vector<string> param);
    ECDAG* Encode();
    ECDAG* Decode(vector<int> from, vector<int> to);
    void Place(vector<vector<int>>& group);
};

#endif
