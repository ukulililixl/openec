#include "NativeRS.hh"
//#include <iostream>
//#include <isa-l.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/time.h>

NativeRS::NativeRS(){};

bool NativeRS::initialize(int n, int k) {
  _n = n;
  _k = k;
  _m = n - k;
  
  gf_gen_rs_matrix((unsigned char*)this->_encode_matrix, _n, _k); // vandermonde matrix, aplpha=2
  ec_init_tables(_k, _m, &this->_encode_matrix[_k * _k], this->_gftbl);
  
  return true;
}

bool NativeRS::construct(uint8_t **data, uint8_t **code, int32_t dataLen) {
  ec_encode_data(dataLen, _k, _m, this->_gftbl, data, code);
  return true;
}

bool NativeRS::check(int fidx) {
  // start from the first idx
  uint8_t emat[_k * _k];
  int32_t pemat = 0;
  for(int32_t i=0; i<_n; i++) {
  	if(i!=fidx) {
  		memcpy(emat+pemat*_k, _encode_matrix+i*_k, _k);
  		pemat++;
  	}
  	if (pemat >= _k) break;
  }
  // 2. calculate the invert matrix.
  uint8_t dmat[_k*_k];
  gf_invert_matrix(emat, dmat, _k);
  
  // choose the line
  uint8_t rmat[1*_k];
  memcpy(rmat, _encode_matrix+fidx*_k, _k);
  
  for(int32_t j=0; j<_k; j++) {
  	fmat[j] = 0;
  	for(int32_t l=0; l<_k; l++) {
  		fmat[j] ^= gf_mul(rmat[l], dmat[l*_k+j]);
  	}
  	//      printf("%d ", fmat[i*_k+j]);
  }
  //ec_init_tables(_k, 1, fmat, tmp_gftbl);
  return true;
}

bool NativeRS::decode(uint8_t** avail, int32_t anum, uint8_t** toret, int32_t tnum, int32_t dataLen) {
  ec_init_tables(_k, 1, fmat, tmp_gftbl);
  ec_encode_data(dataLen, anum, tnum, tmp_gftbl, avail, toret);
  return true;
}

//bool NativeRS::decode(uint8_t** avail, int32_t anum, uint8_t** toret, int32_t tnum, int32_t dataLen, uint8_t* matrix) {
//  ec_init_tables(_k, 1, matrix, tmp_gftbl);
//  ec_encode_data(dataLen, anum, tnum, tmp_gftbl, avail, toret);
//  return true;
//}
