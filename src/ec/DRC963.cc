#include "DRC963.hh"

DRC963::DRC963(int n, int k, int w, int opt, vector<string> param) {
  _n = n;
  _k = k;
  _w = w;
  _locality = true;
  _opt = opt;

  _m = _n - _k;
  _r = atoi(param[0].c_str());;
  _nr = _w;
  _chunk_num_per_node = _m;
  _sys_chunk_num = _k * _chunk_num_per_node;
  _enc_chunk_num = _m * _chunk_num_per_node;
  _total_chunk_num = _sys_chunk_num + _enc_chunk_num;
}

ECDAG* DRC963::Encode(){
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

ECDAG* DRC963::Decode(vector<int> from, vector<int> to) {
  ECDAG* ecdag = new ECDAG();
  int corruptsid;
  corruptsid = to[0]/_w;
  cout << "corruptsid = " << corruptsid << endl;
  int tmp = _total_chunk_num;
  if (corruptsid == 0) {
    // for group1
    // encode 12 13 14 with coef 1 16 17
    vector<int> g1subdata = {12, 13, 14};
    vector<int> g1subcoef = {1, 16, 17};
    int tmp1 = tmp; tmp++;
    ecdag->Join(tmp1, g1subdata, g1subcoef);
//    ecdag->BindY(tmp1, g1subdata[0]);
    // encode 15 16 17 with coef 1 32 51
    vector<int> g1subdata2 = {15, 16, 17};
    vector<int> g1subcoef2 = {1, 32, 51};
    int tmp2 = tmp; tmp++;
    ecdag->Join(tmp2, g1subdata2, g1subcoef2);
//    ecdag->BindY(tmp2, g1subdata2[0]);
    // encode 9 10 11 tmpg1 tmpg2 with matrix:
    /*  1 8 15 1 1
     *  70 170 134 1 0
     *  186 2 184 0 1
     */
    // to obtain tmp3 tmp4 tmp5
    int tmp3=tmp++;
    int tmp4=tmp++;
    int tmp5=tmp++;
    vector<int> g1data = {9, 10, 11, tmp1, tmp2};
    vector<int> g1coef1 = {1, 8, 15, 1, 1};
    ecdag->Join(tmp3, g1data, g1coef1);
    vector<int> g1coef2 = {70, 170, 134, 1, 0};
    ecdag->Join(tmp4, g1data, g1coef2);
    vector<int> g1coef3 = {186, 2, 184, 0, 1};
    ecdag->Join(tmp5, g1data, g1coef3);
    
    vector<int> bind1 = {tmp3, tmp4, tmp5};
    ecdag->BindX(bind1);
   
    // for group2
    // encode 21 22 23 with coef 141 237 72
    vector<int> g2subdata = {21, 22, 23};
    vector<int> g2subcoef = {141, 237, 72};
    int tmp6=tmp++;
    ecdag->Join(tmp6, g2subdata, g2subcoef);
//    ecdag->BindY(tmp6, g2subdata[0]);
    // encode 24 25 26 with coef 112 141 112
    vector<int> g2subdata2 = {24, 25, 26};
    vector<int> g2subcoef2 = {112, 141, 112};
    int tmp7=tmp++;
    ecdag->Join(tmp7, g2subdata2, g2subcoef2);
//    ecdag->BindY(tmp7, g2subdata2[0]);
    // encode 18 19 20 tmp6 tmp7 with matrix to obtain tmp8 tmp9 tmp10
    vector<int> g2data = {18, 19, 20, tmp6, tmp7};
    vector<int> g2coef1 = {1, 1, 1, 0, 0};
    int tmp8=tmp++;
    ecdag->Join(tmp8, g2data, g2coef1);
    vector<int> g2coef2 = {166,104,245,1,0};
    int tmp9=tmp++;
    ecdag->Join(tmp9, g2data, g2coef2);
    vector<int> g2coef3 = {122,167,122,0,1};
    int tmp10=tmp++;
    ecdag->Join(tmp10, g2data, g2coef3);

    vector<int> bind2 = {tmp8, tmp9, tmp10};
    ecdag->BindX(bind2);

    // final decode
    // first encode 3 4 5 6 7 8 tmp3 tmp4 tmp5 tmp8 tmp9 tmp10 to obtain tmp11 tmp12 tmp13
    vector<int> decodedata={3, 4, 5, 6, 7, 8, tmp3, tmp4, tmp5, tmp8, tmp9, tmp10};
    
    vector<int> decodecoef1={1,2,3,1,4,5,1,0,0,1,0,0};
    int tmp11=tmp++;
    ecdag->Join(tmp11, decodedata, decodecoef1);
    
    vector<int> decodecoef2={168,174,175,158,38,207,0,1,0,0,1,0};
    int tmp12=tmp++;
    ecdag->Join(tmp12, decodedata, decodecoef2);
    
    vector<int> decodecoef3={55,79,89,132,70,174,0,0,1,0,0,1};
    int tmp13=tmp++;
    ecdag->Join(tmp13, decodedata, decodecoef3);

    vector<int> bind3 = {tmp11, tmp12, tmp13};
    ecdag->BindX(bind3);

    // set up final decode matrix
    int dec_mat[9] = {1,1,1,43,133,189,10,42,10};
    // reverse the matrix
    int r_dec_mat[9]; 
    jerasure_invert_matrix(dec_mat, r_dec_mat, 3, 8);
    // now we use the coef in r_dec_mat to repair the original data
    vector<int> finaldata = {tmp11, tmp12, tmp13};
    
    vector<int> finalcoef1 = {r_dec_mat[0], r_dec_mat[1], r_dec_mat[2]};
    ecdag->Join(to[0], finaldata, finalcoef1);

    vector<int> finalcoef2 = {r_dec_mat[3], r_dec_mat[4], r_dec_mat[5]};
    ecdag->Join(to[1], finaldata, finalcoef2);
 
    vector<int> finalcoef3 = {r_dec_mat[6], r_dec_mat[7], r_dec_mat[8]};
    ecdag->Join(to[2], finaldata, finalcoef3);
    ecdag->BindX(to);
  } else if (corruptsid == 1) {
    // for group1
    // encode 12 13 14 with coef 1 16 17
    vector<int> g1subdata = {12, 13, 14};
    vector<int> g1subcoef = {1, 16, 17};
    int tmp1 = tmp; tmp++;
    ecdag->Join(tmp1, g1subdata, g1subcoef);
    // encode 15 16 17 with coef 1 32 51
    vector<int> g1subdata2 = {15, 16, 17};
    vector<int> g1subcoef2 = {1, 32, 51};
    int tmp2 = tmp; tmp++;
    ecdag->Join(tmp2, g1subdata2, g1subcoef2);
    // encode 9 10 11 tmpg1 tmpg2 with matrix:
    /*  1 8 15 1 1
     *  70 170 134 1 0
     *  186 2 184 0 1
     */
    // to obtain tmp3 tmp4 tmp5
    int tmp3=tmp++;
    int tmp4=tmp++;
    int tmp5=tmp++;
    vector<int> g1data = {9, 10, 11, tmp1, tmp2};
    vector<int> g1coef1 = {1, 8, 15, 1, 1};
    ecdag->Join(tmp3, g1data, g1coef1);
    vector<int> g1coef2 = {70, 170, 134, 1, 0};
    ecdag->Join(tmp4, g1data, g1coef2);
    vector<int> g1coef3 = {186, 2, 184, 0, 1};
    ecdag->Join(tmp5, g1data, g1coef3);
    
    vector<int> bind1 = {tmp3, tmp4, tmp5};
    ecdag->BindX(bind1);
   
    // for group2
    // encode 21 22 23 with coef 141 237 72
    vector<int> g2subdata = {21, 22, 23};
    vector<int> g2subcoef = {141, 237, 72};
    int tmp6=tmp++;
    ecdag->Join(tmp6, g2subdata, g2subcoef);
    // encode 24 25 26 with coef 112 141 112
    vector<int> g2subdata2 = {24, 25, 26};
    vector<int> g2subcoef2 = {112, 141, 112};
    int tmp7=tmp++;
    ecdag->Join(tmp7, g2subdata2, g2subcoef2);
    // encode 18 19 20 tmp6 tmp7 with matrix to obtain tmp8 tmp9 tmp10
    vector<int> g2data = {18, 19, 20, tmp6, tmp7};
    vector<int> g2coef1 = {1, 1, 1, 0, 0};
    int tmp8=tmp++;
    ecdag->Join(tmp8, g2data, g2coef1);
    vector<int> g2coef2 = {166,104,245,1,0};
    int tmp9=tmp++;
    ecdag->Join(tmp9, g2data, g2coef2);
    vector<int> g2coef3 = {122,167,122,0,1};
    int tmp10=tmp++;
    ecdag->Join(tmp10, g2data, g2coef3);

    vector<int> bind2 = {tmp8, tmp9, tmp10};
    ecdag->BindX(bind2);

    // final decode
    // first encode 0 1 2 6 7 8 tmp3 tmp4 tmp5 tmp8 tmp9 tmp10 to obtain tmp11 tmp12 tmp13
    vector<int> decodedata={0, 1, 2, 6, 7, 8, tmp3, tmp4, tmp5, tmp8, tmp9, tmp10};
    
    vector<int> decodecoef1={1,1,1,1,4,5,1,0,0,1,0,0};
    int tmp11=tmp++;
    ecdag->Join(tmp11, decodedata, decodecoef1);
    
    vector<int> decodecoef2={43,133,189,158,38,207,0,1,0,0,1,0};
    int tmp12=tmp++;
    ecdag->Join(tmp12, decodedata, decodecoef2);
    
    vector<int> decodecoef3={10,42,10,132,70,174,0,0,1,0,0,1};
    int tmp13=tmp++;
    ecdag->Join(tmp13, decodedata, decodecoef3);

    vector<int> bind3 = {tmp11, tmp12, tmp13};
    ecdag->BindX(bind3);

    // set up final decode matrix
    int dec_mat[9] = {1,2,3,168,174,175,55,79,89};
    // reverse the matrix
    int r_dec_mat[9]; 
    jerasure_invert_matrix(dec_mat, r_dec_mat, 3, 8);
    // now we use the coef in r_dec_mat to repair the original data
    vector<int> finaldata = {tmp11, tmp12, tmp13};
    
    vector<int> finalcoef1 = {r_dec_mat[0], r_dec_mat[1], r_dec_mat[2]};
    ecdag->Join(to[0], finaldata, finalcoef1);

    vector<int> finalcoef2 = {r_dec_mat[3], r_dec_mat[4], r_dec_mat[5]};
    ecdag->Join(to[1], finaldata, finalcoef2);
 
    vector<int> finalcoef3 = {r_dec_mat[6], r_dec_mat[7], r_dec_mat[8]};
    ecdag->Join(to[2], finaldata, finalcoef3);
    ecdag->BindX(to);
  } else if (corruptsid == 2) {
    // for group1
    // encode 12 13 14 with coef 1 16 17
    vector<int> g1subdata = {12, 13, 14};
    vector<int> g1subcoef = {1, 16, 17};
    int tmp1 = tmp; tmp++;
    ecdag->Join(tmp1, g1subdata, g1subcoef);
    // encode 15 16 17 with coef 1 32 51
    vector<int> g1subdata2 = {15, 16, 17};
    vector<int> g1subcoef2 = {1, 32, 51};
    int tmp2 = tmp; tmp++;
    ecdag->Join(tmp2, g1subdata2, g1subcoef2);
    // encode 9 10 11 tmpg1 tmpg2 with matrix:
    /*  1 8 15 1 1
     *  70 170 134 1 0
     *  186 2 184 0 1
     */
    // to obtain tmp3 tmp4 tmp5
    int tmp3=tmp++;
    int tmp4=tmp++;
    int tmp5=tmp++;
    vector<int> g1data = {9, 10, 11, tmp1, tmp2};
    vector<int> g1coef1 = {1, 8, 15, 1, 1};
    ecdag->Join(tmp3, g1data, g1coef1);
    vector<int> g1coef2 = {70, 170, 134, 1, 0};
    ecdag->Join(tmp4, g1data, g1coef2);
    vector<int> g1coef3 = {186, 2, 184, 0, 1};
    ecdag->Join(tmp5, g1data, g1coef3);
    
    vector<int> bind1 = {tmp3, tmp4, tmp5};
    ecdag->BindX(bind1);
   
    // for group2
    // encode 21 22 23 with coef 141 237 72
    vector<int> g2subdata = {21, 22, 23};
    vector<int> g2subcoef = {141, 237, 72};
    int tmp6=tmp++;
    ecdag->Join(tmp6, g2subdata, g2subcoef);
    // encode 24 25 26 with coef 112 141 112
    vector<int> g2subdata2 = {24, 25, 26};
    vector<int> g2subcoef2 = {112, 141, 112};
    int tmp7=tmp++;
    ecdag->Join(tmp7, g2subdata2, g2subcoef2);
    // encode 18 19 20 tmp6 tmp7 with matrix to obtain tmp8 tmp9 tmp10
    vector<int> g2data = {18, 19, 20, tmp6, tmp7};
    vector<int> g2coef1 = {1, 1, 1, 0, 0};
    int tmp8=tmp++;
    ecdag->Join(tmp8, g2data, g2coef1);
    vector<int> g2coef2 = {166,104,245,1,0};
    int tmp9=tmp++;
    ecdag->Join(tmp9, g2data, g2coef2);
    vector<int> g2coef3 = {122,167,122,0,1};
    int tmp10=tmp++;
    ecdag->Join(tmp10, g2data, g2coef3);

    vector<int> bind2 = {tmp8, tmp9, tmp10};
    ecdag->BindX(bind2);
    // final decode
    // first encode 0 1 2 3 4 5 tmp3 tmp4 tmp5 tmp8 tmp9 tmp10 to obtain tmp11 tmp12 tmp13
    vector<int> decodedata={0, 1, 2, 3, 4, 5, tmp3, tmp4, tmp5, tmp8, tmp9, tmp10};
    
    vector<int> decodecoef1={1,1,1,1,2,3,1,0,0,1,0,0};
    int tmp11=tmp++;
    ecdag->Join(tmp11, decodedata, decodecoef1);
    
    vector<int> decodecoef2={43,133,189,168,174,175,0,1,0,0,1,0};
    int tmp12=tmp++;
    ecdag->Join(tmp12, decodedata, decodecoef2);
    
    vector<int> decodecoef3={10,42,10,55,79,89,0,0,1,0,0,1};
    int tmp13=tmp++;
    ecdag->Join(tmp13, decodedata, decodecoef3);

    vector<int> bind3 = {tmp11, tmp12, tmp13};
    ecdag->BindX(bind3);

    // set up final decode matrix
    int dec_mat[9] = {1,4,5,158,38,207,132,70,174};
    // reverse the matrix
    int r_dec_mat[9]; 
    jerasure_invert_matrix(dec_mat, r_dec_mat, 3, 8);
    // now we use the coef in r_dec_mat to repair the original data
    vector<int> finaldata = {tmp11, tmp12, tmp13};
    
    vector<int> finalcoef1 = {r_dec_mat[0], r_dec_mat[1], r_dec_mat[2]};
    ecdag->Join(to[0], finaldata, finalcoef1);

    vector<int> finalcoef2 = {r_dec_mat[3], r_dec_mat[4], r_dec_mat[5]};
    ecdag->Join(to[1], finaldata, finalcoef2);
 
    vector<int> finalcoef3 = {r_dec_mat[6], r_dec_mat[7], r_dec_mat[8]};
    ecdag->Join(to[2], finaldata, finalcoef3);
    ecdag->BindX(to);
  } else if (corruptsid == 3) {
    // for group0
    // encode 3 4 5 with coef 1 2 3
    vector<int> g0subdata = {3, 4, 5};
    vector<int> g0subcoef = {1, 2, 3};
    int tmp1 = tmp; tmp++;
    ecdag->Join(tmp1, g0subdata, g0subcoef);
    // encode 6 7 8 with coef 1,4,5
    vector<int> g0subdata2 = {6, 7, 8};
    vector<int> g0subcoef2 = {1, 4, 5};
    int tmp2 = tmp; tmp++;
    ecdag->Join(tmp2, g0subdata2, g0subcoef2);
    // encode 0 1 2 tmp1 tmp2 with matrix:
    /*  1,1,1,1,1
     *  70,82,143,1,0
     *  186,71,186,0,1
     */
    // to obtain tmp3 tmp4 tmp5
    int tmp3=tmp++;
    int tmp4=tmp++;
    int tmp5=tmp++;
    vector<int> g0data = {0, 1, 2, tmp1, tmp2};
    vector<int> g0coef1 = {1, 1, 1, 1, 1};
    ecdag->Join(tmp3, g0data, g0coef1);
    vector<int> g0coef2 = {70,82,143,1,0};
    ecdag->Join(tmp4, g0data, g0coef2);
    vector<int> g0coef3 = {186,71,186,0,1};
    ecdag->Join(tmp5, g0data, g0coef3);
    
    vector<int> bind1 = {tmp3, tmp4, tmp5};
    ecdag->BindX(bind1);
   
    // for group2
    // encode 21 22 23 with coef 224,58,122
    vector<int> g2subdata = {21, 22, 23};
    vector<int> g2subcoef = {224,58,122};
    int tmp6=tmp++;
    ecdag->Join(tmp6, g2subdata, g2subcoef);
    // encode 24 25 26 with coef 192,224,192
    vector<int> g2subdata2 = {24, 25, 26};
    vector<int> g2subcoef2 = {192,224,192};
    int tmp7=tmp++;
    ecdag->Join(tmp7, g2subdata2, g2subcoef2);
    // encode 18 19 20 tmp6 tmp7 with matrix to obtain tmp8 tmp9 tmp10
    vector<int> g2data = {18, 19, 20, tmp6, tmp7};
    vector<int> g2coef1 = {1,1,1,0,0};
    int tmp8=tmp++;
    ecdag->Join(tmp8, g2data, g2coef1);
    vector<int> g2coef2 = {166,104,245,1,0};
    int tmp9=tmp++;
    ecdag->Join(tmp9, g2data, g2coef2);
    vector<int> g2coef3 = {122,167,122,0,1};
    int tmp10=tmp++;
    ecdag->Join(tmp10, g2data, g2coef3);

    vector<int> bind2 = {tmp8, tmp9, tmp10};
    ecdag->BindX(bind2);

    // final decode
    // first encode tmp3 tmp4 tmp5 12 13 14 15 16 17 tmp8 tmp9 tmp10 to obtain tmp11 tmp12 tmp13
    vector<int> decodedata={tmp3, tmp4, tmp5, 12, 13, 14, 15, 16, 17, tmp8, tmp9, tmp10};
    
    vector<int> decodecoef1={1,0,0,1,16,17,1,32,51,1,0,0};
    int tmp11=tmp++;
    ecdag->Join(tmp11, decodedata, decodecoef1);
    
    vector<int> decodecoef2={0,1,0,20,108,102,84,145,47,0,1,0};
    int tmp12=tmp++;
    ecdag->Join(tmp12, decodedata, decodecoef2);
    
    vector<int> decodecoef3={0,0,1,19,77,62,120,114,224,0,0,1};
    int tmp13=tmp++;
    ecdag->Join(tmp13, decodedata, decodecoef3);

    vector<int> bind3 = {tmp11, tmp12, tmp13};
    ecdag->BindX(bind3);

    // set up final decode matrix
    int dec_mat[9] = {1,8,15,4,20,30,6,40,34};
    // reverse the matrix
    int r_dec_mat[9]; 
    jerasure_invert_matrix(dec_mat, r_dec_mat, 3, 8);
    // now we use the coef in r_dec_mat to repair the original data
    vector<int> finaldata = {tmp11, tmp12, tmp13};
    
    vector<int> finalcoef1 = {r_dec_mat[0], r_dec_mat[1], r_dec_mat[2]};
    ecdag->Join(to[0], finaldata, finalcoef1);

    vector<int> finalcoef2 = {r_dec_mat[3], r_dec_mat[4], r_dec_mat[5]};
    ecdag->Join(to[1], finaldata, finalcoef2);
 
    vector<int> finalcoef3 = {r_dec_mat[6], r_dec_mat[7], r_dec_mat[8]};
    ecdag->Join(to[2], finaldata, finalcoef3);
    ecdag->BindX(to);
  } else if (corruptsid == 4) {
    // for group0
    // encode 3 4 5 with coef 1 2 3
    vector<int> g0subdata = {3, 4, 5};
    vector<int> g0subcoef = {1, 2, 3};
    int tmp1 = tmp; tmp++;
    ecdag->Join(tmp1, g0subdata, g0subcoef);
    // encode 6 7 8 with coef 1,4,5
    vector<int> g0subdata2 = {6, 7, 8};
    vector<int> g0subcoef2 = {1, 4, 5};
    int tmp2 = tmp; tmp++;
    ecdag->Join(tmp2, g0subdata2, g0subcoef2);
    // encode 0 1 2 tmp1 tmp2 with matrix:
    /*  1,1,1,1,1
     *  70,82,143,1,0
     *  186,71,186,0,1
     */
    // to obtain tmp3 tmp4 tmp5
    int tmp3=tmp++;
    int tmp4=tmp++;
    int tmp5=tmp++;
    vector<int> g0data = {0, 1, 2, tmp1, tmp2};
    vector<int> g0coef1 = {1, 1, 1, 1, 1};
    ecdag->Join(tmp3, g0data, g0coef1);
    vector<int> g0coef2 = {70,82,143,1,0};
    ecdag->Join(tmp4, g0data, g0coef2);
    vector<int> g0coef3 = {186,71,186,0,1};
    ecdag->Join(tmp5, g0data, g0coef3);
    
    vector<int> bind1 = {tmp3, tmp4, tmp5};
    ecdag->BindX(bind1);
   
    // for group2
    // encode 21 22 23 with coef 224,58,122
    vector<int> g2subdata = {21, 22, 23};
    vector<int> g2subcoef = {224,58,122};
    int tmp6=tmp++;
    ecdag->Join(tmp6, g2subdata, g2subcoef);
    // encode 24 25 26 with coef 192,224,192
    vector<int> g2subdata2 = {24, 25, 26};
    vector<int> g2subcoef2 = {192,224,192};
    int tmp7=tmp++;
    ecdag->Join(tmp7, g2subdata2, g2subcoef2);
    // encode 18 19 20 tmp6 tmp7 with matrix to obtain tmp8 tmp9 tmp10
    vector<int> g2data = {18, 19, 20, tmp6, tmp7};
    vector<int> g2coef1 = {1,1,1,0,0};
    int tmp8=tmp++;
    ecdag->Join(tmp8, g2data, g2coef1);
    vector<int> g2coef2 = {166,104,245,1,0};
    int tmp9=tmp++;
    ecdag->Join(tmp9, g2data, g2coef2);
    vector<int> g2coef3 = {122,167,122,0,1};
    int tmp10=tmp++;
    ecdag->Join(tmp10, g2data, g2coef3);

    vector<int> bind2 = {tmp8, tmp9, tmp10};
    ecdag->BindX(bind2);

    // final decode
    // first encode tmp3 tmp4 tmp5 9 10 11 15 16 17 tmp8 tmp9 tmp10 to obtain tmp11 tmp12 tmp13
    vector<int> decodedata={tmp3, tmp4, tmp5, 9, 10, 11, 15, 16, 17, tmp8, tmp9, tmp10};
    
    vector<int> decodecoef1={1,0,0,1,8,15,1,32,51,1,0,0};
    int tmp11=tmp++;
    ecdag->Join(tmp11, decodedata, decodecoef1);
    
    vector<int> decodecoef2={0,1,0,4,20,30,84,145,47,0,1,0};
    int tmp12=tmp++;
    ecdag->Join(tmp12, decodedata, decodecoef2);
    
    vector<int> decodecoef3={0,0,1,6,40,34,120,114,224,0,0,1};
    int tmp13=tmp++;
    ecdag->Join(tmp13, decodedata, decodecoef3);

    vector<int> bind3 = {tmp11, tmp12, tmp13};
    ecdag->BindX(bind3);

    // set up final decode matrix
    int dec_mat[9] = {1,16,17,20,108,102,19,77,62};
    // reverse the matrix
    int r_dec_mat[9]; 
    jerasure_invert_matrix(dec_mat, r_dec_mat, 3, 8);
    // now we use the coef in r_dec_mat to repair the original data
    vector<int> finaldata = {tmp11, tmp12, tmp13};
    
    vector<int> finalcoef1 = {r_dec_mat[0], r_dec_mat[1], r_dec_mat[2]};
    ecdag->Join(to[0], finaldata, finalcoef1);

    vector<int> finalcoef2 = {r_dec_mat[3], r_dec_mat[4], r_dec_mat[5]};
    ecdag->Join(to[1], finaldata, finalcoef2);
 
    vector<int> finalcoef3 = {r_dec_mat[6], r_dec_mat[7], r_dec_mat[8]};
    ecdag->Join(to[2], finaldata, finalcoef3);
    ecdag->BindX(to);
  } else if (corruptsid == 5) {
    // for group0
    // encode 3 4 5 with coef 1 2 3
    vector<int> g0subdata = {3, 4, 5};
    vector<int> g0subcoef = {1, 2, 3};
    int tmp1 = tmp; tmp++;
    ecdag->Join(tmp1, g0subdata, g0subcoef);
    // encode 6 7 8 with coef 1,4,5
    vector<int> g0subdata2 = {6, 7, 8};
    vector<int> g0subcoef2 = {1, 4, 5};
    int tmp2 = tmp; tmp++;
    ecdag->Join(tmp2, g0subdata2, g0subcoef2);
    // encode 0 1 2 tmp1 tmp2 with matrix:
    /*  1,1,1,1,1
     *  70,82,143,1,0
     *  186,71,186,0,1
     */
    // to obtain tmp3 tmp4 tmp5
    int tmp3=tmp++;
    int tmp4=tmp++;
    int tmp5=tmp++;
    vector<int> g0data = {0, 1, 2, tmp1, tmp2};
    vector<int> g0coef1 = {1, 1, 1, 1, 1};
    ecdag->Join(tmp3, g0data, g0coef1);
    vector<int> g0coef2 = {70,82,143,1,0};
    ecdag->Join(tmp4, g0data, g0coef2);
    vector<int> g0coef3 = {186,71,186,0,1};
    ecdag->Join(tmp5, g0data, g0coef3);
    
    vector<int> bind1 = {tmp3, tmp4, tmp5};
    ecdag->BindX(bind1);
   
    // for group2
    // encode 21 22 23 with coef 224,58,122
    vector<int> g2subdata = {21, 22, 23};
    vector<int> g2subcoef = {224,58,122};
    int tmp6=tmp++;
    ecdag->Join(tmp6, g2subdata, g2subcoef);
    // encode 24 25 26 with coef 192,224,192
    vector<int> g2subdata2 = {24, 25, 26};
    vector<int> g2subcoef2 = {192,224,192};
    int tmp7=tmp++;
    ecdag->Join(tmp7, g2subdata2, g2subcoef2);
    // encode 18 19 20 tmp6 tmp7 with matrix to obtain tmp8 tmp9 tmp10
    vector<int> g2data = {18, 19, 20, tmp6, tmp7};
    vector<int> g2coef1 = {1,1,1,0,0};
    int tmp8=tmp++;
    ecdag->Join(tmp8, g2data, g2coef1);
    vector<int> g2coef2 = {166,104,245,1,0};
    int tmp9=tmp++;
    ecdag->Join(tmp9, g2data, g2coef2);
    vector<int> g2coef3 = {122,167,122,0,1};
    int tmp10=tmp++;
    ecdag->Join(tmp10, g2data, g2coef3);

    vector<int> bind2 = {tmp8, tmp9, tmp10};
    ecdag->BindX(bind2);

    // final decode
    // first encode tmp3 tmp4 tmp5 9 10 11 12 13 14 tmp8 tmp9 tmp10 to obtain tmp11 tmp12 tmp13
    vector<int> decodedata={tmp3, tmp4, tmp5, 9, 10, 11, 12, 13, 14, tmp8, tmp9, tmp10};
    
    vector<int> decodecoef1={1,0,0,1,8,15,1,16,17,1,0,0};
    int tmp11=tmp++;
    ecdag->Join(tmp11, decodedata, decodecoef1);
    
    vector<int> decodecoef2={0,1,0,4,20,30,20,108,102,0,1,0};
    int tmp12=tmp++;
    ecdag->Join(tmp12, decodedata, decodecoef2);
    
    vector<int> decodecoef3={0,0,1,6,40,34,19,77,62,0,0,1};
    int tmp13=tmp++;
    ecdag->Join(tmp13, decodedata, decodecoef3);

    vector<int> bind3 = {tmp11, tmp12, tmp13};
    ecdag->BindX(bind3);

    // set up final decode matrix
    int dec_mat[9] = {1,32,51,84,145,47,120,114,224};
    // reverse the matrix
    int r_dec_mat[9]; 
    jerasure_invert_matrix(dec_mat, r_dec_mat, 3, 8);
    // now we use the coef in r_dec_mat to repair the original data
    vector<int> finaldata = {tmp11, tmp12, tmp13};
    
    vector<int> finalcoef1 = {r_dec_mat[0], r_dec_mat[1], r_dec_mat[2]};
    ecdag->Join(to[0], finaldata, finalcoef1);

    vector<int> finalcoef2 = {r_dec_mat[3], r_dec_mat[4], r_dec_mat[5]};
    ecdag->Join(to[1], finaldata, finalcoef2);
 
    vector<int> finalcoef3 = {r_dec_mat[6], r_dec_mat[7], r_dec_mat[8]};
    ecdag->Join(to[2], finaldata, finalcoef3);
    ecdag->BindX(to);
  } else if (corruptsid == 6) {
    // for group0
    // encode 3 4 5 with coef 79,207,74
    vector<int> g0subdata = {3, 4, 5};
    vector<int> g0subcoef = {79,207,74};
    int tmp1 = tmp; tmp++;
    ecdag->Join(tmp1, g0subdata, g0subcoef);
    // encode 6 7 8 with coef 131,116,2
    vector<int> g0subdata2 = {6, 7, 8};
    vector<int> g0subcoef2 = {131,116,2};
    int tmp2 = tmp; tmp++;
    ecdag->Join(tmp2, g0subdata2, g0subcoef2);
    // encode 0 1 2 tmp1 tmp2 with matrix:
    /*  1,1,1,0,0
     *  37,239,126,1,0
     *  52,206,70,0,1
     */
    // to obtain tmp3 tmp4 tmp5
    int tmp3=tmp++;
    int tmp4=tmp++;
    int tmp5=tmp++;
    vector<int> g0data = {0, 1, 2, tmp1, tmp2};
    vector<int> g0coef1 = {1,1,1,0,0};
    ecdag->Join(tmp3, g0data, g0coef1);
    vector<int> g0coef2 = {37,239,126,1,0};
    ecdag->Join(tmp4, g0data, g0coef2);
    vector<int> g0coef3 = {52,206,70,0,1};
    ecdag->Join(tmp5, g0data, g0coef3);
    
    vector<int> bind1 = {tmp3, tmp4, tmp5};
    ecdag->BindX(bind1);
   
    // for group1
    // encode 12 13 14 with coef 56,23,159
    vector<int> g2subdata = {12, 13, 14};
    vector<int> g2subcoef = {56,23,159};
    int tmp6=tmp++;
    ecdag->Join(tmp6, g2subdata, g2subcoef);
    // encode 15 16 17 with coef 25,198,222
    vector<int> g2subdata2 = {15, 16, 17};
    vector<int> g2subcoef2 = {25,198,222};
    int tmp7=tmp++;
    ecdag->Join(tmp7, g2subdata2, g2subcoef2);
    // encode 9 10 11 tmp6 tmp7 with matrix to obtain tmp8 tmp9 tmp10
    vector<int> g2data = {9, 10, 11, tmp6, tmp7};
    vector<int> g2coef1 = {28,80,90,1,1};
    int tmp8=tmp++;
    ecdag->Join(tmp8, g2data, g2coef1);
    vector<int> g2coef2 = {253,36,48,0,1};
    int tmp9=tmp++;
    ecdag->Join(tmp9, g2data, g2coef2);
    vector<int> g2coef3 = {17,71,218,1,0};
    int tmp10=tmp++;
    ecdag->Join(tmp10, g2data, g2coef3);

    vector<int> bind2 = {tmp8, tmp9, tmp10};
    ecdag->BindX(bind2);

    // final decode
    // first encode tmp3 tmp4 tmp5 9 10 11 12 13 14 tmp8 tmp9 tmp10 to obtain tmp11 tmp12 tmp13
    vector<int> decodedata={tmp3, tmp4, tmp5, tmp8, tmp9, tmp10, 21, 22, 23, 24, 25, 26};
    
    vector<int> decodecoef1={1,0,0,1,0,0,197,121,197,83,170,150};
    int tmp11=tmp++;
    ecdag->Join(tmp11, decodedata, decodecoef1);
    
    vector<int> decodecoef2={0,1,0,0,1,0,12,23,183,25,128,152};
    int tmp12=tmp++;
    ecdag->Join(tmp12, decodedata, decodecoef2);
    
    vector<int> decodecoef3={0,0,1,0,0,1,1,178,239,114,231,19};
    int tmp13=tmp++;
    ecdag->Join(tmp13, decodedata, decodecoef3);

    vector<int> bind3 = {tmp11, tmp12, tmp13};
    ecdag->BindX(bind3);

    // set up final decode matrix
    int dec_mat[9] = {151,210,82,48,120,81,71,155,186};
    // reverse the matrix
    int r_dec_mat[9]; 
    jerasure_invert_matrix(dec_mat, r_dec_mat, 3, 8);
    // now we use the coef in r_dec_mat to repair the original data
    vector<int> finaldata = {tmp11, tmp12, tmp13};
    
    vector<int> finalcoef1 = {r_dec_mat[0], r_dec_mat[1], r_dec_mat[2]};
    ecdag->Join(to[0], finaldata, finalcoef1);

    vector<int> finalcoef2 = {r_dec_mat[3], r_dec_mat[4], r_dec_mat[5]};
    ecdag->Join(to[1], finaldata, finalcoef2);
 
    vector<int> finalcoef3 = {r_dec_mat[6], r_dec_mat[7], r_dec_mat[8]};
    ecdag->Join(to[2], finaldata, finalcoef3);
    ecdag->BindX(to);
  } else if (corruptsid == 7) {
    // for group0
    // encode 3 4 5 with coef 79,207,74
    vector<int> g0subdata = {3, 4, 5};
    vector<int> g0subcoef = {79,207,74};
    int tmp1 = tmp; tmp++;
    ecdag->Join(tmp1, g0subdata, g0subcoef);
    // encode 6 7 8 with coef 131,116,2
    vector<int> g0subdata2 = {6, 7, 8};
    vector<int> g0subcoef2 = {131,116,2};
    int tmp2 = tmp; tmp++;
    ecdag->Join(tmp2, g0subdata2, g0subcoef2);
    // encode 0 1 2 tmp1 tmp2 with matrix:
    /*  1,1,1,0,0
     *  37,239,126,1,0
     *  52,206,70,0,1
     */
    // to obtain tmp3 tmp4 tmp5
    int tmp3=tmp++;
    int tmp4=tmp++;
    int tmp5=tmp++;
    vector<int> g0data = {0, 1, 2, tmp1, tmp2};
    vector<int> g0coef1 = {1,1,1,0,0};
    ecdag->Join(tmp3, g0data, g0coef1);
    vector<int> g0coef2 = {37,239,126,1,0};
    ecdag->Join(tmp4, g0data, g0coef2);
    vector<int> g0coef3 = {52,206,70,0,1};
    ecdag->Join(tmp5, g0data, g0coef3);
    
    vector<int> bind1 = {tmp3, tmp4, tmp5};
    ecdag->BindX(bind1);
   
    // for group1
    // encode 12 13 14 with coef 56,23,159
    vector<int> g2subdata = {12, 13, 14};
    vector<int> g2subcoef = {56,23,159};
    int tmp6=tmp++;
    ecdag->Join(tmp6, g2subdata, g2subcoef);
    // encode 15 16 17 with coef 25,198,222
    vector<int> g2subdata2 = {15, 16, 17};
    vector<int> g2subcoef2 = {25,198,222};
    int tmp7=tmp++;
    ecdag->Join(tmp7, g2subdata2, g2subcoef2);
    // encode 9 10 11 tmp6 tmp7 with matrix to obtain tmp8 tmp9 tmp10
    vector<int> g2data = {9, 10, 11, tmp6, tmp7};
    vector<int> g2coef1 = {28,80,90,1,1};
    int tmp8=tmp++;
    ecdag->Join(tmp8, g2data, g2coef1);
    vector<int> g2coef2 = {253,36,48,0,1};
    int tmp9=tmp++;
    ecdag->Join(tmp9, g2data, g2coef2);
    vector<int> g2coef3 = {17,71,218,1,0};
    int tmp10=tmp++;
    ecdag->Join(tmp10, g2data, g2coef3);

    vector<int> bind2 = {tmp8, tmp9, tmp10};
    ecdag->BindX(bind2);

    // final decode
    // first encode tmp3 tmp4 tmp5 9 10 11 12 13 14 tmp8 tmp9 tmp10 to obtain tmp11 tmp12 tmp13
    vector<int> decodedata={tmp3, tmp4, tmp5, tmp8, tmp9, tmp10, 18, 19, 20, 24, 25, 26};
    
    vector<int> decodecoef1={1,0,0,1,0,0,151,210,82,83,170,150};
    int tmp11=tmp++;
    ecdag->Join(tmp11, decodedata, decodecoef1);
    
    vector<int> decodecoef2={0,1,0,0,1,0,48,120,81,25,128,152};
    int tmp12=tmp++;
    ecdag->Join(tmp12, decodedata, decodecoef2);
    
    vector<int> decodecoef3={0,0,1,0,0,1,71,155,186,114,231,19};
    int tmp13=tmp++;
    ecdag->Join(tmp13, decodedata, decodecoef3);

    vector<int> bind3 = {tmp11, tmp12, tmp13};
    ecdag->BindX(bind3);

    // set up final decode matrix
    int dec_mat[9] = {197,121,197,12,23,183,1,178,239};
    // reverse the matrix
    int r_dec_mat[9]; 
    jerasure_invert_matrix(dec_mat, r_dec_mat, 3, 8);
    // now we use the coef in r_dec_mat to repair the original data
    vector<int> finaldata = {tmp11, tmp12, tmp13};
    
    vector<int> finalcoef1 = {r_dec_mat[0], r_dec_mat[1], r_dec_mat[2]};
    ecdag->Join(to[0], finaldata, finalcoef1);

    vector<int> finalcoef2 = {r_dec_mat[3], r_dec_mat[4], r_dec_mat[5]};
    ecdag->Join(to[1], finaldata, finalcoef2);
 
    vector<int> finalcoef3 = {r_dec_mat[6], r_dec_mat[7], r_dec_mat[8]};
    ecdag->Join(to[2], finaldata, finalcoef3);
    ecdag->BindX(to);
  } else if (corruptsid == 8) {
    // for group0
    // encode 3 4 5 with coef 79,207,74
    vector<int> g0subdata = {3, 4, 5};
    vector<int> g0subcoef = {79,207,74};
    int tmp1 = tmp; tmp++;
    ecdag->Join(tmp1, g0subdata, g0subcoef);
    // encode 6 7 8 with coef 131,116,2
    vector<int> g0subdata2 = {6, 7, 8};
    vector<int> g0subcoef2 = {131,116,2};
    int tmp2 = tmp; tmp++;
    ecdag->Join(tmp2, g0subdata2, g0subcoef2);
    // encode 0 1 2 tmp1 tmp2 with matrix:
    /*  1,1,1,0,0
     *  37,239,126,1,0
     *  52,206,70,0,1
     */
    // to obtain tmp3 tmp4 tmp5
    int tmp3=tmp++;
    int tmp4=tmp++;
    int tmp5=tmp++;
    vector<int> g0data = {0, 1, 2, tmp1, tmp2};
    vector<int> g0coef1 = {1,1,1,0,0};
    ecdag->Join(tmp3, g0data, g0coef1);
    vector<int> g0coef2 = {37,239,126,1,0};
    ecdag->Join(tmp4, g0data, g0coef2);
    vector<int> g0coef3 = {52,206,70,0,1};
    ecdag->Join(tmp5, g0data, g0coef3);
    
    vector<int> bind1 = {tmp3, tmp4, tmp5};
    ecdag->BindX(bind1);
   
    // for group1
    // encode 12 13 14 with coef 56,23,159
    vector<int> g2subdata = {12, 13, 14};
    vector<int> g2subcoef = {56,23,159};
    int tmp6=tmp++;
    ecdag->Join(tmp6, g2subdata, g2subcoef);
    // encode 15 16 17 with coef 25,198,222
    vector<int> g2subdata2 = {15, 16, 17};
    vector<int> g2subcoef2 = {25,198,222};
    int tmp7=tmp++;
    ecdag->Join(tmp7, g2subdata2, g2subcoef2);
    // encode 9 10 11 tmp6 tmp7 with matrix to obtain tmp8 tmp9 tmp10
    vector<int> g2data = {9, 10, 11, tmp6, tmp7};
    vector<int> g2coef1 = {28,80,90,1,1};
    int tmp8=tmp++;
    ecdag->Join(tmp8, g2data, g2coef1);
    vector<int> g2coef2 = {253,36,48,0,1};
    int tmp9=tmp++;
    ecdag->Join(tmp9, g2data, g2coef2);
    vector<int> g2coef3 = {17,71,218,1,0};
    int tmp10=tmp++;
    ecdag->Join(tmp10, g2data, g2coef3);

    vector<int> bind2 = {tmp8, tmp9, tmp10};
    ecdag->BindX(bind2);

    // final decode
    // first encode tmp3 tmp4 tmp5 9 10 11 12 13 14 tmp8 tmp9 tmp10 to obtain tmp11 tmp12 tmp13
    vector<int> decodedata={tmp3, tmp4, tmp5, tmp8, tmp9, tmp10, 18, 19, 20, 21, 22, 23};
    
    vector<int> decodecoef1={1,0,0,1,0,0,151,210,82,197,121,197};
    int tmp11=tmp++;
    ecdag->Join(tmp11, decodedata, decodecoef1);
    
    vector<int> decodecoef2={0,1,0,0,1,0,48,120,81,12,23,183};
    int tmp12=tmp++;
    ecdag->Join(tmp12, decodedata, decodecoef2);
    
    vector<int> decodecoef3={0,0,1,0,0,1,71,155,186,1,178,239};
    int tmp13=tmp++;
    ecdag->Join(tmp13, decodedata, decodecoef3);

    vector<int> bind3 = {tmp11, tmp12, tmp13};
    ecdag->BindX(bind3);

    // set up final decode matrix
    int dec_mat[9] = {83,170,150,25,128,152,114,231,19};
    // reverse the matrix
    int r_dec_mat[9]; 
    jerasure_invert_matrix(dec_mat, r_dec_mat, 3, 8);
    // now we use the coef in r_dec_mat to repair the original data
    vector<int> finaldata = {tmp11, tmp12, tmp13};
    
    vector<int> finalcoef1 = {r_dec_mat[0], r_dec_mat[1], r_dec_mat[2]};
    ecdag->Join(to[0], finaldata, finalcoef1);

    vector<int> finalcoef2 = {r_dec_mat[3], r_dec_mat[4], r_dec_mat[5]};
    ecdag->Join(to[1], finaldata, finalcoef2);
 
    vector<int> finalcoef3 = {r_dec_mat[6], r_dec_mat[7], r_dec_mat[8]};
    ecdag->Join(to[2], finaldata, finalcoef3);
    ecdag->BindX(to);
  }
  return ecdag;
}

void DRC963::Place(vector<vector<int>>& group) {
//  // xiaolu add for hierarchical data centers
//  int groupsize = _m;
//  int groupnum = _n / groupsize;
//  for (int groupid=0; groupid<groupnum; groupid++) {
//    vector<int> curgroup;
//    for (int idx=groupid*groupsize; idx<(groupid+1)*groupsize && idx<_n; idx++) {
//      curgroup.push_back(idx);
//    }
//    group.push_back(curgroup);
//  }
}

void DRC963::generate_encoding_matrix() {
  memset(_enc_matrix, 0, _total_chunk_num * _sys_chunk_num * sizeof(int));
  for (int i=0; i<_sys_chunk_num; i++) {
    _enc_matrix[i*_sys_chunk_num+i]=1;
  }
  for (int i=_sys_chunk_num; i<_total_chunk_num; i++) {
    int temp = 1;
    for (int j=0; j<_k; j++) {
      if (i % 3 == 0) {
        _enc_matrix[i*_sys_chunk_num+3*j]=temp;
      } else if (i % 3 == 1) {
        _enc_matrix[i*_sys_chunk_num+3*j+1]=temp;
      } else {
        _enc_matrix[i*_sys_chunk_num+3*j+2]=temp;
      }
      temp = Computation::singleMulti(i-_sys_chunk_num+1, temp, 8);
    }
  }
}
