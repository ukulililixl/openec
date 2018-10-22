#include "RSCONV.hh"

RSCONV::RSCONV(int n, int k, int w, bool locality, int opt, vector<string> param) {
  _n = n;
  _k = k;
  _w = w;
  _locality = locality;
  _opt = opt;

  _m = _n - _k;
  memset(_encode_matrix, 0, (_k+_m) * _k * sizeof(int));
  if(RSCONV_DEBUG_ENABLE) cout << "RSCONV::constructor ends" << endl; 
}

ECDAG* RSCONV::Encode() {
  ECDAG* ecdag = new ECDAG();
  vector<int> data;
  vector<int> code;
  for (int i=0; i<_k; i++) data.push_back(i);
  for (int i=_k; i<_n; i++) code.push_back(i);
  if (RSCONV_DEBUG_ENABLE) {
    cout << "RSCONV::Encode.data:";
    for (int i=0; i<data.size(); i++) cout << " " << data[i];
    cout << endl;
    cout << "RSCONV::Encode.code:";
    for (int i=0; i<code.size(); i++) cout << " " << code[i];
    cout << endl;
  }
  
  generate_matrix(_encode_matrix, _n, _k, 8);
  for (int i=0; i<_m; i++) {
    vector<int> coef;
    for (int j=0; j<_k; j++) {
      coef.push_back(_encode_matrix[(i+_k)*_k+j]);
    }
    ecdag->Join(code[i], data, coef);
  }
  return ecdag;
}

ECDAG* RSCONV::Decode(vector<int> from, vector<int> to) {
}

void RSCONV::Place(vector<vector<int>>& group) {
}

void RSCONV::generate_matrix(int* matrix, int rows, int cols, int w) {
  int k = cols;
  int n = rows;
  int m = n - k;

  memset(matrix, 0, rows * cols * sizeof(int));
  for(int i=0; i<k; i++) {
    matrix[i*k+i] = 1;
  }

  for (int i=0; i<m; i++) {
    int tmp = 1;
    for (int j=0; j<k; j++) {
      matrix[(i+k)*cols+j] = tmp;
      tmp = Computation::singleMulti(tmp, i+1, w);
    }
  }
}
