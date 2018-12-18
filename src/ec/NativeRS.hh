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
    unsigned char* _encode_matrix; 
    unsigned char* _gftbl;
    unsigned char* tmp_gftbl;
    unsigned char* fmat;

  public:
    NativeRS();
    ~NativeRS();
    void generate_matrix(unsigned char* matrix, int rows, int cols, int w);
    bool initialize(int n, int k);
    bool construct(uint8_t **data, uint8_t **code, int32_t dataLen);
    bool check(int fidx);
    bool decode(uint8_t** avail, int32_t anum, uint8_t** toret, int32_t tnum, int32_t dataLen);
    void dump(unsigned char* mat, int rows, int cols);
};

#endif
