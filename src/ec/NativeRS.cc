#include "NativeRS.hh"
//#include <iostream>
//#include <isa-l.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/time.h>

NativeRS::NativeRS(){};
NativeRS::~NativeRS() {
  if(_encode_matrix) free(_encode_matrix);
  if(_gftbl) free(_gftbl);
  if(fmat) free(fmat);
}

bool NativeRS::initialize(int n, int k) {
  _n = n;
  _k = k;
  _m = n - k;

  _encode_matrix=(unsigned char*)calloc(_n*_k, sizeof(unsigned char));
  _gftbl=(unsigned char*)calloc(_n*_k*32, sizeof(unsigned char));
  generate_matrix(_encode_matrix, _n, _k, 8);
  //gf_gen_rs_matrix(_encode_matrix, _n, _k); // vandermonde matrix, aplpha=2
  ec_init_tables(_k, _m, &this->_encode_matrix[_k * _k], this->_gftbl);
  
  return true;
}

void NativeRS::generate_matrix(unsigned char* matrix, int rows, int cols, int w) {
  int k = cols;
  int n = rows;
  int m = n - k;
  
  memset(matrix, 0, rows * cols * sizeof(unsigned char));
  for(int i=0; i<k; i++) {
    matrix[i*k+i] = (char)1;
  }
  
  for (int i=0; i<m; i++) {
    int tmp = 1;
    for (int j=0; j<k; j++) {
      matrix[(i+k)*cols+j] = (char)tmp;
      tmp = Computation::singleMulti(tmp, i+1, w);
    }
  }
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
  
  fmat = (unsigned char*)calloc(1*_k, sizeof(unsigned char));
  for(int32_t j=0; j<_k; j++) {
  	fmat[j] = 0;
  	for(int32_t l=0; l<_k; l++) {
  		fmat[j] ^= gf_mul(rmat[l], dmat[l*_k+j]);
  	}
  }
  tmp_gftbl = (unsigned char*)calloc(1*_k*32, sizeof(unsigned char));
  ec_init_tables(_k, 1, fmat, tmp_gftbl);
  return true;
}

bool NativeRS::decode(uint8_t** avail, int32_t anum, uint8_t** toret, int32_t tnum, int32_t dataLen) {
  ec_encode_data(dataLen, anum, tnum, tmp_gftbl, avail, toret);
  return true;
}

void NativeRS::dump(unsigned char* mat, int rows, int cols) {
  for (int i=0; i<rows; i++) {
    for (int j=0; j<cols; j++) {
      printf("%d ", mat[i*cols+j]);
    }
    printf("\n");
  }
}
