#include "IA.hh"

IA::IA(int n, int k, int w, int opt, vector<string> param) {
  _n = n;
  _k = k;
  _w = w;
  _opt = opt;

  _chunk_num_per_node = _k;
  _sys_chunk_num = _k * _chunk_num_per_node;
  _enc_chunk_num = (_n - _k) * _chunk_num_per_node;
  _total_chunk_num = _sys_chunk_num + _enc_chunk_num;
}

ECDAG* IA::Encode() {
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
    for (int j=0; j<_sys_chunk_num;j++) {
      coef.push_back(_final_enc_matrix[i*_sys_chunk_num +j]);
    }
    ecdag->Join(code[i], data, coef);
  }
  if (code.size() > 1) ecdag->BindX(code);
  return ecdag;
}

void IA::generate_encoding_matrix() {
  //  cout << "generate_encoding_matrix" << endl;
  
  int* UMat = (int *)calloc(_k * _k, sizeof(int));
  int* VMat = (int *)calloc(_k * _k, sizeof(int));
  int* PMat = (int *)calloc(_k * _k, sizeof(int));
  int* invUMat = (int *)calloc(_k * _k, sizeof(int));
  int* invPMat = (int *)calloc(_k * _k, sizeof(int));

  // generate matrix P first
  square_cauchy_matrix(PMat, _k);
//  cout << "PMat is :" << endl;
//  print_matrix(PMat, _k, _k);

  // generate V as I
  memset(VMat, 0, _k*_k*sizeof(int));
  for (int i=0; i<_k; i++) {
    VMat[i*_k+i] = 1;
  }
//  cout << "VMat is :" << endl;
//  print_matrix(VMat, _k, _k);

  // U= 1/k * P
  int greekK = 2;
  int KMinus = galois_inverse(greekK, 8);
  for (int i = 0; i < _k; i++) {
    for (int j = 0; j < _k; j++) {
//      UMat[i * _k + j] = galois_single_multiply(KMinus, PMat[i * _k + j], 8);
      UMat[i * _k + j] = Computation::singleMulti(KMinus, PMat[i*_k+j], 8);
    }
  }
//  cout << "UMat is :" << endl;
//  print_matrix(UMat, _k, _k);

  // create invUMat and invPMat
  int *tmp_matrix = (int *)calloc(_k * _k, sizeof(int));
  memcpy(tmp_matrix, UMat, _k * _k * sizeof(int));
  jerasure_invert_matrix(tmp_matrix, invUMat, _k, 8);
//  cout << "invUMat is:" << endl;
//  print_matrix(invUMat, _k, _k);
  
  memcpy(tmp_matrix, PMat, _k * _k * sizeof(int));
  jerasure_invert_matrix(tmp_matrix, invPMat, _k, 8);
//  cout << "invPMat is:" << endl;
//  print_matrix(invPMat, _k, _k);

  free(tmp_matrix);

  // Now we create the lower part of the encoding matrix
  // It is composed of (_k * _k) matrices
  memset(_ori_encoding_matrix, 0, _total_chunk_num * _sys_chunk_num * sizeof(int));
  for (int i = 0; i < _sys_chunk_num; i++) {
    _ori_encoding_matrix[i * _sys_chunk_num + i] = 1;
  }

  for (int i = 0; i < _k; i++) {
    for (int j = 0; j < _k; j++) {
      // Generate the square matrix at i-th row and j-th column
      // The way to calculate should be u_i * v_j^t + p[j,i] * I
      int rStart = (_k + j) * _k;
      int cStart = i * _k;
      int *VRow = (int *)calloc(_k, sizeof(int));
      int *URow = (int *)calloc(_k, sizeof(int));
      for (int k = 0; k < _k; k++) {
        VRow[k] = VMat[k * _k + i];
	URow[k] = UMat[k * _k + j];
      }
      int *tmpMat = jerasure_matrix_multiply(URow, VRow, _k, 1, 1, _k, 8);
      for (int32_t k = 0; k < _k; k++) {
        tmpMat[k * _k + k] ^= PMat[i * _k + j];
      }
      for (int32_t k = 0; k < _k; k++) {
        for (int32_t l = 0; l < _k; l++) {
	  _ori_encoding_matrix[(rStart + k) * _sys_chunk_num + l + cStart] = tmpMat[l * _k + k];
	}
      }
      free(tmpMat);
      free(VRow);
      free(URow);
    }
  }
//  cout << "_ori_encoding_matrix:" << endl;
//  print_matrix(_ori_encoding_matrix, _total_chunk_num, _sys_chunk_num);

  // create dul_enc_matrix
  memset(_dual_enc_matrix, 0, _total_chunk_num * _sys_chunk_num * sizeof(int));
  tmp_matrix = (int *)calloc(_enc_chunk_num * _sys_chunk_num, sizeof(int));
  memcpy(tmp_matrix, _ori_encoding_matrix + _sys_chunk_num * _sys_chunk_num,
           _enc_chunk_num * _sys_chunk_num * sizeof(int));
  jerasure_invert_matrix(tmp_matrix, _dual_enc_matrix, _sys_chunk_num, 8);

  free(tmp_matrix);

  for (int i = 0; i < _sys_chunk_num; i++) {
    _dual_enc_matrix[_sys_chunk_num * _sys_chunk_num + i * _sys_chunk_num + i] = 1;
  }
//  cout << "_dual_enc_matrix:" << endl;
//  print_matrix(_dual_enc_matrix, _total_chunk_num, _sys_chunk_num);

  // create meta data for data reconstruction
  memcpy(_offline_enc_vec, VMat, _k * _k * sizeof(int));
  for (int i = 0; i < _k; i++) {
    for (int j = 0; j < _k; j++) {
      _offline_enc_vec[_k * _k + i * _k + j] = UMat[j * _k + i];
    }
  }

//  cout << "_offline_enc_vec:" << endl;
//  print_matrix(_offline_enc_vec, _n, _k);

  memcpy(_final_enc_matrix, _ori_encoding_matrix + _sys_chunk_num * _sys_chunk_num,
             _enc_chunk_num * _sys_chunk_num * sizeof(int));
//  cout << "_final_enc_matrix = " << endl;
//  print_matrix(_final_enc_matrix, _enc_chunk_num, _sys_chunk_num);

  free(UMat);
  free(VMat);
  free(PMat);
  free(invUMat);
  free(invPMat);
}

void IA::square_cauchy_matrix(int *des, int size) {
  //  cout << "generate square_cachy_matrix" << endl;
  int *Xset = (int *)calloc(size, sizeof(int));
  int *Yset = (int *)calloc(size, sizeof(int));
  for (int i = 0; i < size; i++) {
    Xset[i] = i + 1;
    Yset[i] = i + 1 + size;
  }
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      des[i * size + j] = galois_inverse(Xset[i]^Yset[j], 8);
    }
  }
  free(Xset);
  free(Yset);
}

ECDAG* IA::Decode(vector<int> from, vector<int> to) {
  ECDAG* ecdag = new ECDAG();
  //  cout << "IA::decode" << endl;
  generate_encoding_matrix();
  int rBlkIdx = to[0]/_k;
  generate_decoding_matrix(rBlkIdx);
  cout << "final decoding matrix:" << endl;
//  print_matrix(_recovery_equations, _k, _n-1);
  int tmpidx = _n * _k;
 
  for (int i=0; i<to.size(); i++) {
    int rUnitIdx = to[i];
//    cout << "to recover unit " << rUnitIdx << endl;
//    cout << "to recover block " << rBlkIdx << endl;
    vector<int> rgdata;
    for (int j=0; j<_n; j++) {
      if (rBlkIdx == j) continue;
      vector<int> groupdata;
      vector<int> groupcoef;
      for (int k=0; k<_k; k++) {
        if (_offline_enc_vec[rBlkIdx * _k + k]>0) {
	  groupdata.push_back(j*_k+k);
	  groupcoef.push_back(_offline_enc_vec[rBlkIdx * _k + k]);
	}
      }
      if (groupdata.size() == 1) rgdata.push_back(groupdata[0]);
      else {
        int tmpnode=tmpidx++;
        ecdag->Join(tmpnode, groupdata, groupcoef);
	rgdata.push_back(tmpnode);
      }
    }
    vector<int> rgcoef;
    for (int j=0; j<_n-1; j++) {
      rgcoef.push_back(_recovery_equations[i*(_n-1)+j]);
    }
    ecdag->Join(rUnitIdx, rgdata, rgcoef);
  } 
  ecdag->BindX(to);
  return ecdag;
}

void IA::generate_decoding_matrix(int rBlkIdx) {
  //  cout << "generate decoding matrix" << endl;
  memset(_recovery_equations, 0, _chunk_num_per_node*(_n-1)*sizeof(int));
  int* recvData = (int *)calloc((_n - 1) * _sys_chunk_num, sizeof(int));

  if (rBlkIdx < _k) {
    // it is a source blk
    int survInd = 0;
    for (int i = 0; i < _n; i++) {
      if (i == rBlkIdx) continue;
      for (int j=0; j<_k; j++) {
        if (_offline_enc_vec[rBlkIdx * _k + j] != 0) {
	  for (int k = 0; k < _sys_chunk_num; k++) {
//	    recvData[survInd * _sys_chunk_num + k] ^=
//	        galois_single_multiply(_offline_enc_vec[rBlkIdx * _k + j], 
//		                       _ori_encoding_matrix[(i * _k + j) * _sys_chunk_num + k], 8);
            recvData[survInd * _sys_chunk_num + k] ^=
                Computation::singleMulti(_offline_enc_vec[rBlkIdx * _k + j],
                                         _ori_encoding_matrix[(i * _k + j) * _sys_chunk_num + k], 8);
	  }
	}
      }
      survInd++;
    }

    //
//    cout << "recvData before IA interfere alignment :" << endl;
//    print_matrix(recvData, _n-1, _sys_chunk_num);

    // Now we eliminate the interference alignment
    survInd = 0;
    for (int i = 0; i < _k; i++) {
      _recovery_equations[i * (_n - 1) + i + _k - 1] = 1;
    }
    for (int i = 0; i < _k; i++) {
      if (i == rBlkIdx) continue;
      int coefficient = 0;
      for (int j = 0; j < _k; j++) {
        for (int k = i * _k; k < (i + 1) * _k; k++) { 
	  if ((recvData[survInd * _sys_chunk_num + k] != 0) &&
	      (recvData[(j + _k - 1) * _sys_chunk_num + k] != 0)) {
	    coefficient = recvData[(j + _k - 1) * _sys_chunk_num + k] * 
	        galois_inverse(recvData[survInd * _sys_chunk_num + k], 8);
            _recovery_equations[j * (_n - 1) + survInd] = coefficient;
	    break;
	  }
	}
      }
      survInd++;
    }
//    cout << "After IA elimination, _recovery_equations : " << endl;
//    print_matrix(_recovery_equations, _k, _n-1);

    // Now generate final recover equations
    int *oriSqr = (int *)calloc(_k * _k, sizeof(int));
    int *invOriSqr = (int *)calloc(_k * _k, sizeof(int));
    int rStart = _k - 1;
    int cStart = rBlkIdx * _k;
    for (int i = 0; i < _k; i++) {
      for (int j = 0; j < _k; j++) {
        oriSqr[i * _k + j] =
	    recvData[(rStart + i) * _sys_chunk_num + j + cStart];
      }
    }
    jerasure_invert_matrix(oriSqr, invOriSqr, _k, 8);
    int* temp = _recovery_equations;
    int* tempres = jerasure_matrix_multiply(invOriSqr, temp, _k, _k, _k, _n-1, 8);
    memcpy(_recovery_equations, tempres, _k * (_n-1) * sizeof(int));
//    cout << "Final recovery equations:" << endl;
//    print_matrix(_recovery_equations, _k, _n-1);

    // free
    free(tempres);
    free(oriSqr);
    free(invOriSqr);
  } else {
    // parity repair
    int survInd = 0;
    for (int i = 0; i < _n; i++) {
      if (i == rBlkIdx) continue;
      for (int j = 0; j < _k; j++) {
        if (_offline_enc_vec[rBlkIdx * _k + j] != 0) {
	  for (int k = 0; k < _sys_chunk_num; k++) {
//	    recvData[survInd * _sys_chunk_num + k] ^=
//	        galois_single_multiply(_offline_enc_vec[rBlkIdx * _k + j],
//		                       _dual_enc_matrix[(i * _k + j) * _sys_chunk_num + k], 8);
            recvData[survInd * _sys_chunk_num + k] ^=
                Computation::singleMulti(_offline_enc_vec[rBlkIdx * _k + j],
                                       _dual_enc_matrix[(i * _k + j) * _sys_chunk_num + k], 8);
          }
	}
      }
      survInd++;
    }
    // eliminate
    survInd = _k;
    for (int i = 0; i < _k; i++) {
      _recovery_equations[i * (_n - 1) + i] = 1;
    }
    for (int i = _k; i < _n; i++) {
      if (i == rBlkIdx) continue;
      int coefficient = 0;
      for (int j = 0; j < _k; j++) {
        for (int k = (i - _k) * _k; k < (i + 1 - _k) * _k; k++) {
	  if ((recvData[survInd * _sys_chunk_num + k] != 0) &&
	      (recvData[j * _sys_chunk_num + k] != 0)) {
//	    coefficient = galois_single_multiply(recvData[j * _sys_chunk_num + k], 
//	                                         galois_inverse(recvData[survInd * _sys_chunk_num + k], 8), 
//						 8);
            coefficient = Computation::singleMulti(recvData[j * _sys_chunk_num + k],
                                                 galois_inverse(recvData[survInd * _sys_chunk_num + k], 8),
                                                 8);
	    _recovery_equations[j * (_n - 1) + survInd] = coefficient;
	    break;
	  }
	}
      }
      survInd++;
    }

    // generate final recovery equations
    int* oriSqr = (int *)calloc(_k * _k, sizeof(int));
    int* invOriSqr = (int *)calloc(_k * _k, sizeof(int));
    int rStart = 0;
    int cStart = (rBlkIdx - _k) * _k;
    for (int i = 0; i < _k; i++) {
      for (int j = 0; j < _k; j++) {
        oriSqr[i * _k + j] = recvData[(rStart + i) * _sys_chunk_num + j + cStart];
      }
    }
    jerasure_invert_matrix(oriSqr, invOriSqr, _k, 8);
    int* temp = _recovery_equations;
    int* tempres = jerasure_matrix_multiply(invOriSqr, temp, _k, _k, _k, _n-1, 8);
    memcpy(_recovery_equations, tempres, _k * (_n-1) * sizeof(int));
//    cout << "final recovery matrix:" << endl;
//    print_matrix(_recovery_equations, _k, _n-1);

    free(tempres);
    free(oriSqr);
    free(invOriSqr);

  }

  free(recvData);
}

void IA::Place(vector<vector<int>>& group) {
}

