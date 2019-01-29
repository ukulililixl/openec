#include "WASLRC.hh"

WASLRC::WASLRC(int n, int k, int w, int opt, vector<string> param) {
  _n = n;
  _k = k;
  _w = w;
  _opt = opt;
  // there should be two other parameters in param
  // 0. l (local parity num)
  // 1. r (global parity num)
  cout << param.size() << endl;
  _l = atoi(param[0].c_str());
  _r = atoi(param[1].c_str());

  memset(_encode_matrix, 0, _n * _k * sizeof(int));
}

ECDAG* WASLRC::Encode() {
  ECDAG* ecdag = new ECDAG();
  vector<int> data;
  vector<int> code;
  for (int i=0; i<_k; i++) data.push_back(i);
  for (int i=_k; i<_n; i++) code.push_back(i);
  
  generate_matrix(_encode_matrix, _k, _l, _r, 8); 
  for (int i=0; i<code.size(); i++) {
    vector<int> coef;
    for (int j=0; j<_k; j++) {
      coef.push_back(_encode_matrix[(i+_k)*_k+j]);
    }
    ecdag->Join(code[i], data, coef);
  } 
  if (code.size() > 1) ecdag->BindX(code);
  return ecdag;
}

ECDAG* WASLRC::Decode(vector<int> from, vector<int> to) {
  ECDAG* ecdag = new ECDAG();
  if (to.size() == 1) {
    // can recover by local parity
    int ridx = to[0];
    vector<int> data;
    vector<int> coef;
    int nr = _k/_l;

    if (ridx < _k) {
      // source data
      int gidx = ridx/nr;
      for (int i=0; i<nr; i++) {
        int idxinstripe = gidx * nr + i;
        if (ridx != idxinstripe) {
          data.push_back(idxinstripe);
          coef.push_back(1);
        }
      }
      data.push_back(_k+gidx);
      coef.push_back(1);
    } else if (ridx < (_k+_l)) {
      // local parity
      int gidx = ridx - _k;
      for (int i=0; i<nr; i++) {
        int idxinstripe = gidx*nr+i;
        data.push_back(idxinstripe);
        coef.push_back(1);
      }
    } else {
      // global parity
      generate_matrix(_encode_matrix, _k, _l, _r, 8);
      for (int i=0; i<_k; i++) {
        data.push_back(i);
        coef.push_back(_encode_matrix[ridx*_k+i]);
      }
    }
    ecdag->Join(ridx, data, coef);
  }
  return ecdag;
}

void WASLRC::generate_matrix(int* matrix, int k, int l, int r, int w) {
  int n = k + l + r;
  memset(matrix, 0, n*k*sizeof(int));
  // set first k lines
  for (int i=0; i<k; i++) {
    matrix[i*k+i] = 1;
  }

  // set the following l lines as local parity
  int nr = k/l;
  for (int i=0; i<l; i++) {
    for (int j=0; j<nr; j++) {
      matrix[(k+i)*k + i*nr + j] = 1;
    }
  }

  // set the last r lines
  for (int i=0; i<r; i++) {
    for (int j=0; j<l; j++) {
      int tmp = 1;
      for (int ii=0; ii<nr; ii++) {
        matrix[(k+l+i)*k+j*nr+ii]=tmp;
        tmp = Computation::singleMulti(tmp, i+2, w);
      }
    }
  }
}

void WASLRC::Place(vector<vector<int>>& group) {
}
