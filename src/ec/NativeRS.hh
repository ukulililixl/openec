#ifndef _NativeRS_HH_
#define _NativeRS_HH_

#include "../inc/include.hh"
#include "Computation.hh"
//#include <stdint.h>
//#include <isa-l.h>
#define NativeRS_N_MAX (32)

using namespace std;

class NativeRS {
  private:
    int _n;
    int _k;
    int _m;
    uint8_t _encode_matrix[NativeRS_N_MAX * NativeRS_N_MAX];
    uint8_t _gftbl[NativeRS_N_MAX * NativeRS_N_MAX * 32];
    uint8_t fmat[NativeRS_N_MAX * NativeRS_N_MAX];
  public:
    NativeRS();
    bool initialize(int n, int k);
    bool construct(uint8_t **data, uint8_t **code, int32_t dataLen);
    bool check(int fidx);
    bool decode(uint8_t** avail, int32_t anum, uint8_t** toret, int32_t tnum, int32_t dataLen);
};

#endif
