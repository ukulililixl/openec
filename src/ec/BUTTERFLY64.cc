#include "BUTTERFLY64.hh"

BUTTERFLY64::BUTTERFLY64(int n, int k, int w, int opt, vector<string> param) {
  cout << "BUTTERRFLY64" << endl;
  _n = n;
  _k = k;
  _w = w;
  _opt = opt;

  _m = _n - _k;
  _r = 3;
  _nr = _n / _r;

  _chunk_num_per_node = 8;
  _sys_chunk_num = _k * _chunk_num_per_node;
  _enc_chunk_num = _m * _chunk_num_per_node;
  _total_chunk_num = _sys_chunk_num + _enc_chunk_num;

  _tmp = _total_chunk_num;
}

ECDAG* BUTTERFLY64::Encode() {
  ECDAG* ecdag = new ECDAG();
  vector<int> data;
  vector<int> code;
  for (int i=0; i<_k; i++) {
    for (int j=0; j<_w; j++) data.push_back(i*_w+j);
  }
  for (int i=_k; i<_n; i++) {
    for (int j=0; j<_w; j++) code.push_back(i*_w+j);
  }

  generate_encoding_matrix();
  for (int i=0; i<_enc_chunk_num; i++) {
    vector<int> coef;
    for (int j=0; j<_sys_chunk_num; j++) {
      coef.push_back(_enc_matrix[(_sys_chunk_num+i) * _sys_chunk_num + j]);
    }
    ecdag->Join(code[i], data, coef);
  }
  ecdag->BindX(code);

  return ecdag;
}

ECDAG* BUTTERFLY64::Decode(vector<int> from, vector<int> to) {
  ECDAG* ecdag = new ECDAG();
  int rBlkIdx = to[0] / _chunk_num_per_node;

  if (rBlkIdx == 0 || rBlkIdx == 1 || rBlkIdx ==2 || rBlkIdx == 4) {
    vector<int> data;
    vector<int> off;
    int matrix[160];
    if (rBlkIdx == 0) {
      off={0,1,2,3};
      int a[160] = { 1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,
                     0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,
                     0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,
                     0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,
                     1,0,0,0,1,0,1,0,1,0,0,1,0,0,0,0,0,0,0,1,
                     0,1,0,0,0,1,0,1,0,1,1,1,0,0,0,0,0,0,1,0,
                     0,0,1,0,1,0,0,0,1,1,0,0,0,0,0,0,0,1,0,0,
                     0,0,0,1,0,1,0,0,1,0,0,0,0,0,0,0,1,0,0,0};
      memcpy(matrix, a, 160*sizeof(int));
    } else if (rBlkIdx == 1) {
      off={0,1,6,7};
      int a[160] = { 1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,
                     0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,
                     0,0,1,0,1,0,0,0,1,1,0,0,0,0,0,0,0,1,0,0,
                     0,0,0,1,0,1,0,0,1,0,0,0,0,0,0,0,1,0,0,0,
                     0,0,0,0,0,0,1,0,0,0,0,1,1,0,0,0,0,0,0,1,
                     0,0,0,0,0,0,0,1,0,0,1,1,0,1,0,0,0,0,1,0,
                     0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,
                     0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0};
      memcpy(matrix, a, 160*sizeof(int));
    } else if (rBlkIdx == 2) {
      off={0,3,4,7};
      int a[160] = { 1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,
                     0,0,0,1,0,1,0,0,1,0,0,0,0,0,0,0,1,0,0,0,
                     1,0,1,0,0,0,0,0,0,1,0,0,1,0,0,0,0,1,0,0,
                     0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,
                     0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,
                     0,0,0,1,0,0,0,0,0,0,1,0,0,1,0,1,0,0,1,0,
                     0,0,0,0,0,0,1,0,0,0,0,1,1,0,0,0,0,0,0,1,
                     0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0};
      memcpy(matrix, a, 160*sizeof(int));
    } else if (rBlkIdx == 4) {
      off={4,5,6,7};
      int a[160] = { 0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,1,
                     0,0,0,0,0,1,0,0,0,0,0,1,0,0,1,1,0,0,1,0,
                     0,0,0,0,0,0,1,0,1,0,1,0,1,1,1,0,0,1,0,0,
                     0,0,0,0,0,0,0,1,0,1,0,1,1,0,0,1,1,0,0,0,
                     1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,
                     0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,
                     0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,
                     0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0};
      memcpy(matrix, a, 160*sizeof(int));
    }
    for (int i=0; i<_n; i++) {
      if (i == rBlkIdx) continue;
      for (int j=0; j<off.size(); j++) {
        int cid = i * _chunk_num_per_node + off[j];
        data.push_back(cid);
      }
    }
    for (int i=0; i<to.size(); i++) {
      vector<int> coef;
      for (int j=0; j<20; j++) {
        int c = matrix[i*20+j];
        coef.push_back(c);
      }
      ecdag->Join(to[i], data, coef);
    }  
    ecdag->BindX(to);
  } else if (rBlkIdx == 3) {
    vector<int> off1={1,3,5,7}; // for nodeid==5;
    vector<int> off2={0,2,4,6}; // for other nodes

    vector<int> data;
    for (int i=0; i<_n; i++) {
      if (i==rBlkIdx) continue;
      else if (i == 5) {
        for (int j=0; j<off1.size(); j++) {
          int cid = i * _chunk_num_per_node + off1[j];
          data.push_back(cid);
        }
      } else {
        for (int j=0; j<off2.size(); j++) {
          int cid = i * _chunk_num_per_node + off2[j];
          data.push_back(cid);
        }
      }
    }

    int a[160] = { 1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,
                   1,0,0,1,1,1,0,0,0,0,0,0,1,0,0,0,1,0,0,0,
                   0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0,
                   1,0,1,0,0,0,0,0,0,1,0,0,1,0,0,0,0,1,0,0,
                   0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,
                   0,0,1,1,0,0,1,0,0,0,0,0,0,1,1,1,0,0,1,0,
                   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,
                   0,0,0,0,0,0,1,0,0,0,0,1,1,0,0,0,0,0,0,1};
    for (int i=0; i<to.size(); i++) {
      vector<int> coef;
      for (int j=0; j<20; j++) {
        int c = a[i*20+j];
        coef.push_back(c);
      }
      ecdag->Join(to[i], data, coef);
    }
    ecdag->BindX(to);
  } else if (rBlkIdx == 5) {
    vector<int> data;

    vector<int> off0={0,1,2,3}; // for nodeid=0
    for (int i=0; i<off0.size(); i++) data.push_back(off0[i]);

    // for nodeid=1 2 3
    for (int sid = 1; sid <=3; sid++) {
      int a[32];
      vector<int> curdata;
      for (int i=0; i<_chunk_num_per_node; i++) {
        int cid = sid * _chunk_num_per_node + i;
        curdata.push_back(cid);
      }
      if (sid == 1) {
        int a1[32] = { 1,0,0,0,1,0,0,0,
                       0,1,0,0,0,1,0,0,
                       0,0,1,0,0,0,1,0,
                       0,0,0,1,0,0,0,1};
        memcpy(a, a1, 32*sizeof(int));
      } else if (sid == 2) {
        int a2[32] = { 0,0,0,1,0,1,0,1,
                       0,0,1,0,1,0,1,0,
                       0,1,0,0,0,0,0,1,
                       1,0,0,0,0,0,1,0};
        memcpy(a, a2, 32*sizeof(int));
      } else if (sid == 3) {
        int a3[32] = { 0,0,0,1,1,0,0,1,
                       0,0,1,0,1,1,1,0,
                       0,1,0,0,0,0,1,1,
                       1,0,0,0,0,0,0,1};
        memcpy(a, a3, 32*sizeof(int));
      }
      vector<int> curres;
      for (int i=0; i<4; i++) {
        vector<int> curcoef;
        for (int j=0; j<8; j++) curcoef.push_back(a[i*8+j]);
        ecdag->Join(_tmp, curdata, curcoef);
        curres.push_back(_tmp);
        data.push_back(_tmp);
        _tmp++;
      }
      ecdag->BindX(curres);
    }

    vector<int> off4={4,5,6,7}; // for nodeid=4
    for (int i=0; i<off4.size(); i++) data.push_back(4*_chunk_num_per_node+off4[i]);
    
    for (int i=0; i<data.size(); i++) cout << data[i] << " ";
    cout << endl;

    int a[160] = { 0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,1,0,0,0,1,
                   0,0,0,0,0,0,1,0,0,0,0,1,0,0,1,1,0,0,1,0,
                   0,0,0,0,0,1,0,0,1,0,1,0,1,1,1,0,0,1,0,0,
                   0,0,0,0,1,0,0,0,0,1,0,1,1,0,0,1,1,0,0,0,
                   0,0,0,1,0,0,0,1,1,0,0,0,1,0,0,0,0,0,0,0,
                   0,0,1,0,0,0,1,0,0,1,0,0,0,1,0,0,0,0,0,0,
                   0,1,0,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,0,0,
                   1,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0};
    for (int i=0; i<to.size(); i++) {
      vector<int> coef;
      for (int j=0; j<20; j++) coef.push_back(a[i*20+j]);
      ecdag->Join(to[i], data, coef);
    }
    ecdag->BindX(to);
  }
  return ecdag;
}


void BUTTERFLY64::generate_encoding_matrix() {
  memset(_enc_matrix, 0, BUTTERFLY_MAX*BUTTERFLY_MAX*sizeof(int));

  for (int i=0; i<_sys_chunk_num; i++) {
    _enc_matrix[i*_sys_chunk_num+i]=1;
  }

  int temp=0;
  for (int i=_sys_chunk_num; i<_sys_chunk_num+8; i++) {
    for (int j=0; j<_k; j++) {
      if (i%8 == temp) {
        _enc_matrix[i*_sys_chunk_num+8*j+temp] = 1;
      }
    }
    temp++;
  }

  int a[8*32] = {0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0,0,0,0,
                 0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,
                 0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,1,1,0,0,0,0,
                 0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,
                 0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,1,0,1,0,0,0,1,1,0,0,1,
                 0,0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,1,0,1,0,0,0,1,0,1,1,1,0,
                 0,1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,1,0,0,0,0,1,1,
                 1,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1,0,1,0,0,0,0,0,0,1};
  memcpy(_enc_matrix+40*32, a, 8*32*sizeof(int));
}

void BUTTERFLY64::Place(vector<vector<int>>& group) {
}

