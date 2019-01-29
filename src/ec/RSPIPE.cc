#include "RSPIPE.hh"

RSPIPE::RSPIPE(int n, int k, int w, int opt, vector<string> param) {
  _n = n;
  _k = k;
  _w = w;
  _opt = opt;

  _m = _n - _k;
  memset(_encode_matrix, 0, (_k+_m)*_m*sizeof(int));
}

ECDAG* RSPIPE::Encode() {
  ECDAG* ecdag = new ECDAG();
  vector<int> data;
  vector<int> code;
  for (int i=0; i<_k; i++) data.push_back(i);
  for (int i=_k; i<_n; i++) code.push_back(i);

  generate_matrix(_encode_matrix, _n, _k, 8);
  for (int i=0; i<_m; i++) {
    vector<int> coef;
    for (int j=0; j<_k; j++) {
      coef.push_back(_encode_matrix[(i+_k)*_k+j]);
    }
    ecdag->Join(code[i], data, coef);
  }
  if (code.size() > 0) ecdag->BindX(code);
  return ecdag;
}

ECDAG* RSPIPE::Decode(vector<int> from, vector<int> to) {
  ECDAG* ecdag = new ECDAG();

  generate_matrix(_encode_matrix, _n, _k, 8);

  int _select_matrix[_k*_k];
  for (int i=0; i<_k; i++) {
    int sidx = from[i];
    memcpy(_select_matrix + i * _k,
           _encode_matrix + sidx * _k,
	   sizeof(int) * _k);
  }
  
  int _invert_matrix[_k*_k];
  jerasure_invert_matrix(_select_matrix, _invert_matrix, _k, _k);

  int tmpname = _k + _m;

  for (int i=0; i<to.size(); i++) {
    int ridx = to[i];
    int _select_vector[_k];
    memcpy(_select_vector,
           _encode_matrix + ridx * _k,
	   _k * sizeof(int));
    int* _coef_vector = jerasure_matrix_multiply(
             _select_vector, _invert_matrix, 1, _k, _k, _k, 8);
    // prepare data and coef
    deque<int> dataqueue;
    deque<int> coefqueue;
    for (int j=0; j<_k; j++) {
      dataqueue.push_back(from[j]);
      coefqueue.push_back(_coef_vector[j]);
    }

    while(dataqueue.size()>=2) {
      vector<int> datav;
      vector<int> coefv;

      for (int j=0; j<2; j++) {
        int tmpd(dataqueue.front());
	dataqueue.pop_front();
	int tmpc(coefqueue.front());
	coefqueue.pop_front();
	datav.push_back(tmpd);
	coefv.push_back(tmpc);
      }
      int toadd;
      if (dataqueue.size() == 0) {
        toadd = ridx;
      } else {
        toadd = tmpname++;
      }
      ecdag->Join(toadd, datav, coefv);
      ecdag->BindY(toadd, datav[1]);
      dataqueue.push_front(toadd);
      coefqueue.push_front(1);
    }
  }
  return ecdag;
} 

void RSPIPE::Place(vector<vector<int>>& group){}

void RSPIPE::generate_matrix(int* matrix, int rows, int cols, int w) {
  int k = cols;
  int n = rows;
  int m = n - k;
  memset(matrix, 0, rows * cols *sizeof(int));
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

