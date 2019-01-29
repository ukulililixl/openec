#include "DRC643.hh"

DRC643::DRC643(int n, int k, int w, int opt, vector<string> param) {
  _n = n;
  _k = k;
  _w = w;
  _locality = true;
  _opt = opt;

  _r = atoi(param[0].c_str());
  _m = _n - _k;
  _nr = _n / _r;
  _chunk_num_per_node = _m;
  _sys_chunk_num = _k * _chunk_num_per_node;
  _enc_chunk_num = _m * _chunk_num_per_node;
  _total_chunk_num = _sys_chunk_num + _enc_chunk_num;
}

ECDAG* DRC643::Encode() {
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
      coef.push_back(_enc_matrix[(_sys_chunk_num + i)*_sys_chunk_num + j]);
    }
    ecdag->Join(code[i], data, coef);
  }
  ecdag->BindX(code);

  return ecdag;
}

ECDAG* DRC643::Decode(vector<int> from, vector<int> to){
  ECDAG* ecdag = new ECDAG();

  int rBlkIdx = to[0] / _chunk_num_per_node;
  int tmp = _total_chunk_num;
  if (rBlkIdx == 0) {
    vector<int> finaldata; finaldata.push_back(2); finaldata.push_back(3);

    vector<int> g1data = {4, 5, 7};
    vector<int> g1coef1 = {12, 0, 0};
    vector<int> g1coef2 = {0, 1, 1};
    vector<int> tobind1;
    ecdag->Join(tmp, g1data, g1coef1);
    finaldata.push_back(tmp); tobind1.push_back(tmp); tmp++;
    ecdag->Join(tmp, g1data, g1coef2);
    finaldata.push_back(tmp); tobind1.push_back(tmp); tmp++;
    ecdag->BindX(tobind1);

    vector<int> g2data = {8, 9, 10};
    vector<int> g2coef1 = {8, 0, 1};
    vector<int> g2coef2 = {0, 1, 0};
    vector<int> tobind2;
    ecdag->Join(tmp, g2data, g2coef1);
    finaldata.push_back(tmp); tobind2.push_back(tmp); tmp++;
    ecdag->Join(tmp, g2data, g2coef2);
    finaldata.push_back(tmp); tobind2.push_back(tmp); tmp++;
    ecdag->BindX(tobind2);

    vector<int> finalcoef1 = {187,0,157,0,157,0};
    vector<int> finalcoef2 = {0,1,0,1,0,1};
    ecdag->Join(to[0], finaldata, finalcoef1);
    ecdag->Join(to[1], finaldata, finalcoef2);
    ecdag->BindX(to);
  } else if (rBlkIdx == 1) {
    vector<int> finaldata; finaldata.push_back(0); finaldata.push_back(1);

    vector<int> g1data = {4, 5, 7};
    vector<int> g1coef1 = {12, 0, 0};
    vector<int> g1coef2 = {0, 1, 1};
    vector<int> tobind1;
    ecdag->Join(tmp, g1data, g1coef1);
    finaldata.push_back(tmp); tobind1.push_back(tmp); tmp++;
    ecdag->Join(tmp, g1data, g1coef2);
    finaldata.push_back(tmp); tobind1.push_back(tmp); tmp++;
    ecdag->BindX(tobind1);

    vector<int> g2data = {8, 9, 10};
    vector<int> g2coef1 = {8, 0, 1};
    vector<int> g2coef2 = {0, 1, 0};
    vector<int> tobind2;
    ecdag->Join(tmp, g2data, g2coef1);
    finaldata.push_back(tmp); tobind2.push_back(tmp); tmp++;
    ecdag->Join(tmp, g2data, g2coef2);
    finaldata.push_back(tmp); tobind2.push_back(tmp); tmp++;
    ecdag->BindX(tobind2);
    
    vector<int> finalcoef1 = {123,0,221,0,221,0};
    vector<int> finalcoef2 = {0,1,0,1,0,1};
    ecdag->Join(to[0], finaldata, finalcoef1);
    ecdag->Join(to[1], finaldata, finalcoef2);
    ecdag->BindX(to);
  } else if (rBlkIdx == 2) {
    vector<int> finaldata;

    vector<int> g0data = {0, 1, 3};
    vector<int> g0coef1 = {3, 0, 0};
    vector<int> g0coef2 = {0, 1, 1};
    vector<int> tobind0;
    ecdag->Join(tmp, g0data, g0coef1);
    finaldata.push_back(tmp); tobind0.push_back(tmp); tmp++;
    ecdag->Join(tmp, g0data, g0coef2);
    finaldata.push_back(tmp); tobind0.push_back(tmp); tmp++;
    ecdag->BindX(tobind0);
    
    finaldata.push_back(6); finaldata.push_back(7);

    vector<int> g2data = {8, 9, 10};
    vector<int> g2coef1 = {2, 0, 1};
    vector<int> g2coef2 = {0, 1, 0};
    vector<int> tobind2;
    ecdag->Join(tmp, g2data, g2coef1);
    finaldata.push_back(tmp); tobind2.push_back(tmp); tmp++;
    ecdag->Join(tmp, g2data, g2coef2);
    finaldata.push_back(tmp); tobind2.push_back(tmp); tmp++;
    ecdag->BindX(tobind2);

    vector<int> finalcoef1 = {122,0,3,0,122,0};
    vector<int> finalcoef2 = {0,1,0,1,0,1};
    ecdag->Join(to[0], finaldata, finalcoef1);
    ecdag->Join(to[1], finaldata, finalcoef2);
    ecdag->BindX(to);
  } else if (rBlkIdx == 3) {
    vector<int> finaldata;

    vector<int> g0data = {0, 1, 3};
    vector<int> g0coef1 = {3, 0, 0};
    vector<int> g0coef2 = {0, 1, 1};
    vector<int> tobind0;
    ecdag->Join(tmp, g0data, g0coef1);
    finaldata.push_back(tmp); tobind0.push_back(tmp); tmp++;
    ecdag->Join(tmp, g0data, g0coef2);
    finaldata.push_back(tmp); tobind0.push_back(tmp); tmp++;
    ecdag->BindX(tobind0);
    
    finaldata.push_back(4); finaldata.push_back(5);

    vector<int> g2data = {8, 9, 10};
    vector<int> g2coef1 = {2, 0, 1};
    vector<int> g2coef2 = {0, 1, 0};
    vector<int> tobind2;
    ecdag->Join(tmp, g2data, g2coef1);
    finaldata.push_back(tmp); tobind2.push_back(tmp); tmp++;
    ecdag->Join(tmp, g2data, g2coef2);
    finaldata.push_back(tmp); tobind2.push_back(tmp); tmp++;
    ecdag->BindX(tobind2);

    vector<int> finalcoef1 = {221,0,244,0,221,0};
    vector<int> finalcoef2 = {0,1,0,1,0,1};
    ecdag->Join(to[0], finaldata, finalcoef1);
    ecdag->Join(to[1], finaldata, finalcoef2);
    ecdag->BindX(to);
  } else if (rBlkIdx == 4) {
    vector<int> finaldata;

    vector<int> g0data = {0, 1, 3};
    vector<int> g0coef1 = {143,0,0};
    vector<int> g0coef2 = {0,172,70};
    vector<int> tobind0;
    ecdag->Join(tmp, g0data, g0coef1);
    finaldata.push_back(tmp); tobind0.push_back(tmp); tmp++;
    ecdag->Join(tmp, g0data, g0coef2);
    finaldata.push_back(tmp); tobind0.push_back(tmp); tmp++;
    ecdag->BindX(tobind0);

    vector<int> g1data = {4, 5, 6};
    vector<int> g1coef1 = {3, 0, 5};
    vector<int> g1coef2 = {0, 143, 0};
    vector<int> tobind1;
    ecdag->Join(tmp, g1data, g1coef1);
    finaldata.push_back(tmp); tobind1.push_back(tmp); tmp++;
    ecdag->Join(tmp, g1data, g1coef2);
    finaldata.push_back(tmp); tobind1.push_back(tmp); tmp++;
    ecdag->BindX(tobind1);

    finaldata.push_back(10); finaldata.push_back(11);

    vector<int> finalcoef1 = {1,0,1,0,142,0};
    vector<int> finalcoef2 = {0,1,0,1,0,173};
    ecdag->Join(to[0], finaldata, finalcoef1);
    ecdag->Join(to[1], finaldata, finalcoef2);
    ecdag->BindX(to);
  } else if (rBlkIdx == 5) {
    vector<int> finaldata;

    vector<int> g0data = {0, 1, 3};
    vector<int> g0coef1 = {3,0,0};
    vector<int> g0coef2 = {0,9,10};
    vector<int> tobind0;
    ecdag->Join(tmp, g0data, g0coef1);
    finaldata.push_back(tmp); tobind0.push_back(tmp); tmp++;
    ecdag->Join(tmp, g0data, g0coef2);
    finaldata.push_back(tmp); tobind0.push_back(tmp); tmp++;
    ecdag->BindX(tobind0);

    vector<int> g1data = {4, 5, 6};
    vector<int> g1coef1 = {6, 0, 10};
    vector<int> g1coef2 = {0, 12, 0};
    vector<int> tobind1;
    ecdag->Join(tmp, g1data, g1coef1);
    finaldata.push_back(tmp); tobind1.push_back(tmp); tmp++;
    ecdag->Join(tmp, g1data, g1coef2);
    finaldata.push_back(tmp); tobind1.push_back(tmp); tmp++;
    ecdag->BindX(tobind1);

    finaldata.push_back(8); finaldata.push_back(9);

    vector<int> finalcoef1 = {1,0,1,0,2,0};
    vector<int> finalcoef2 = {0,1,0,1,0,8};
    ecdag->Join(to[0], finaldata, finalcoef1);
    ecdag->Join(to[1], finaldata, finalcoef2);
    ecdag->BindX(to);
  }
  return ecdag;
}

void DRC643::generate_encoding_matrix() {
  memset(_enc_matrix, 0, _total_chunk_num * _sys_chunk_num * sizeof(int));
  for(int i=0; i<_sys_chunk_num; i++) {
    _enc_matrix[i*_sys_chunk_num+i]=1;
  }

  for(int i=_sys_chunk_num; i<_total_chunk_num; i++) {
    int temp=1;
    for(int j=0; j<_k; j++) {
      if(i % 2 == 0){
        _enc_matrix[i*_sys_chunk_num+2*j]=temp;
      }else{
        _enc_matrix[i*_sys_chunk_num+2*j+1]=temp;
      }
      temp = Computation::singleMulti((i-_sys_chunk_num)/_m+1, temp, 8);
    }
  }
}

void DRC643::Place(vector<vector<int>>& group) {
}

