#ifndef _RSBINDX_HH_
#define _RSBINDX_HH_

#include "../inc/include.hh"
#include "Computation.hh"

#include "ECBase.hh"

using namespace std;

#define RSBINDX_N_MAX (32)

#define RSBINDX_DEBUG_ENABLE true

class RSBINDX : public ECBase {
  private:
    int _encode_matrix[RSBINDX_N_MAX * RSBINDX_N_MAX];
    int _m;

    void generate_matrix(int* matrix, int rows, int cols, int w);  // This w is for galois field, which is different from our sub-packetization level _w

  public:
    RSBINDX(int n, int k, int w, int opt, vector<string> param);
 
    ECDAG* Encode();
    ECDAG* Decode(vector<int> from, vector<int> to);
    void Place(vector<vector<int>>& group);
};

#endif
