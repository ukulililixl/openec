#include "Computation.hh"

mutex Computation::_cLock;

int Computation::singleMulti(int a, int b, int w) {
  Computation::_cLock.lock();
  int res = galois_single_multiply(a, b, w);
  Computation::_cLock.unlock();
  return res;
}

void Computation::Multi(char** dst, char** src, int* mat, int rowCnt, int colCnt, int len, string lib) {
  if (lib == "Jerasure") {
    Computation::_cLock.lock();
    jerasure_matrix_encode(colCnt, rowCnt, GF_W, mat, src, dst, len);
    Computation::_cLock.unlock();
  } else {
    // first transfer the mat into char*
    char* imatrix;
    imatrix = (char*)calloc(rowCnt * colCnt, sizeof(char));
    for (int i=0; i<rowCnt * colCnt; i++) {
      char tmpc = mat[i];
      imatrix[i] = tmpc;
    }
    // call isa-l library
    unsigned char itable[32 * rowCnt * colCnt];
    ec_init_tables(colCnt, rowCnt, (unsigned char*)imatrix, itable);
    ec_encode_data(len, colCnt, rowCnt, itable, (unsigned char**)src, (unsigned char**)dst);
  }
}
