#include "common/Config.hh"
#include "ec/ECDAG.hh"
#include "ec/ECPolicy.hh"
#include "ec/ECBase.hh"
#include "ec/ECTask.hh"
#include "ec/NativeRS.hh"
#include "inc/include.hh"

using namespace std;

void usage() {
  cout << "Usage: ./CodeTest " << endl;
  cout << "	1, systype (native/openec)" << endl;
  cout << "	2, n" << endl;
  cout << "	3, k" << endl;
  cout << "	4, blocksizeB" << endl;
  cout << "	5, pktsizeB" << endl;
}

double getCurrentTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec * 1e+6 + (double)tv.tv_usec;
}

int main(int argc, char** argv) {
  if (argc < 6) {
    usage();
    return 0;
  }

  string systype = string(argv[1]);
  int n = atoi(argv[2]);
  int k = atoi(argv[3]);
  int blocksizeB = atoi(argv[4]);
  int pktsizeB = atoi(argv[5]);

  string confpath = "conf/sysSetting.xml";
  Config* conf = new Config(confpath);
  int stripenum = blocksizeB/pktsizeB;
  string ecid = "rs_"+to_string(n)+"_"+to_string(k);

  // we need to prepare the original data
  // prepare data buffer and code buffer
  uint8_t** databuffers = (uint8_t**)calloc(k, sizeof(uint8_t*));
  for (int i=0; i<k; i++) {
    databuffers[i] = (uint8_t*)calloc(pktsizeB, sizeof(uint8_t));
  }
  uint8_t** codebuffers = (uint8_t**)calloc((n-k), sizeof(uint8_t*));
  for (int i=0; i<(n-k); i++) {
    codebuffers[i] = (uint8_t*)calloc(pktsizeB, sizeof(uint8_t));
  }
  uint8_t* oribuffer = (uint8_t*)calloc(pktsizeB, sizeof(uint8_t));

  NativeRS* rscode = nullptr;
  ECPolicy* ecpolicy = nullptr;
  ECBase* ec = nullptr;

  double initEncodeTime=0;
  initEncodeTime -= getCurrentTime();

  // prepare for encode
  ECDAG* encdag = nullptr;
  vector<ECTask*> encodetasks;
  unordered_map<int, char*> encodeBufMap;
  if (systype == "native") {
    rscode = new NativeRS();
    rscode->initialize(n, k);
  } else {
    ecpolicy = conf->_ecPolicyMap[ecid];
    ec = ecpolicy->createECClass();
    encdag = ec->Encode();
    vector<int> toposeq = encdag->toposort();
    for (int i=0; i<toposeq.size(); i++) {
      ECNode* curnode = encdag->getNode(toposeq[i]);
      curnode->parseForClient(encodetasks);
    }
    for (int i=0; i<k; i++) encodeBufMap.insert(make_pair(i, (char*)databuffers[i]));
    for (int i=0; i<(n-k); i++) encodeBufMap.insert(make_pair(k+i, (char*)codebuffers[i]));
  }
  initEncodeTime += getCurrentTime();

  // prepare for decode
  double initDecodeTime=0;
  initDecodeTime -= getCurrentTime();
  int fidx=0;
  ECDAG* decdag = nullptr;
  vector<ECTask*> decodetasks;
  unordered_map<int, char*> decodeBufMap;
  vector<int> availidx;
  vector<int> torecidx;

  if (systype == "native") {
    rscode->check(fidx);
  } else {
    for(int i=0; i<n; i++) {
      if (i == fidx) torecidx.push_back(i);
      else availidx.push_back(i);
    }
    decdag = ec->Decode(availidx, torecidx);
    vector<int> toposeq = decdag->toposort();
    for (int i=0; i<toposeq.size(); i++) {
      ECNode* curnode = decdag->getNode(toposeq[i]);
      curnode->parseForClient(decodetasks);
    }
  }
  initDecodeTime += getCurrentTime();


  // test
  double encodeTime = 0, decodeTime = 0;
  srand((unsigned)1234);
  for (int stripei=0; stripei<stripenum; stripei++) {
    // clean codebuffers
    for (int i=0;i<(n-k); i++) {
      memset(codebuffers[i], 0, pktsizeB);
    }
    // initialize databuffers
    for (int i=0; i<k; i++) {
      for (int j=0; j<pktsizeB; j++) databuffers[i][j] = rand();
    }
    // encode test
    encodeTime -= getCurrentTime();
    if (systype == "native") rscode->construct(databuffers, codebuffers, pktsizeB);
    else {
      for (int taskid = 0; taskid < encodetasks.size(); taskid++) {
        ECTask* compute = encodetasks[taskid];
	vector<int> children = compute->getChildren();
	unordered_map<int, vector<int>> coefMap = compute->getCoefMap();
	int col = children.size();
	int row = coefMap.size();
	vector<int> targets;
	int* matrix = (int*)calloc(row*col, sizeof(int));
        char** data = (char**)calloc(col, sizeof(char*));
	char** code = (char**)calloc(row, sizeof(char*));
	for (int bufIdx=0; bufIdx<children.size(); bufIdx++) {
          int child = children[bufIdx];
	  data[bufIdx] = encodeBufMap[child];
	}
	int codeBufIdx = 0;
	for (auto it: coefMap) {
          int target = it.first;
	  char* codebuf;
	  if (encodeBufMap.find(target) == encodeBufMap.end()) {
            codebuf = (char*)calloc(pktsizeB, sizeof(char));
	    encodeBufMap.insert(make_pair(target, codebuf));
	  } else {
            codebuf = encodeBufMap[target];
	  }
	  code[codeBufIdx] = codebuf;
	  targets.push_back(target);
	  vector<int> curcoef = it.second;
	  for (int j=0; j<col; j++) {
            matrix[codeBufIdx * col + j] = curcoef[j];
	  }
	  codeBufIdx++;
	}
	Computation::Multi(code, data, matrix, row, col, pktsizeB, "Isal");
	free(matrix);
	free(data);
	free(code);
      }
    }
    encodeTime += getCurrentTime();

    // take out fidx buffer
    uint8_t* cpybuf;
    if (fidx < k) cpybuf = databuffers[fidx];
    else cpybuf = codebuffers[fidx-k];
    memcpy(oribuffer, cpybuf, pktsizeB);

    // prepare avail buffer and torec buffer
    // and decodeBufMap
    decodeBufMap.clear();
    uint8_t* availbuffers[k];
    int aidx=0;
    for (int i=0; i<k && aidx<k; i++) {
      if (i == fidx) continue;
      availbuffers[aidx++] = databuffers[i];
      decodeBufMap.insert(make_pair(i, (char*)databuffers[i]));
    }
    for (int i=0; i<(n-k) && aidx<k; i++) {
      if (i+k == fidx) continue;
      availbuffers[aidx++] = codebuffers[i];
      decodeBufMap.insert(make_pair(i+k, (char*)codebuffers[i]));
    }
    uint8_t* toretbuffers[1];
    uint8_t repairbuf[pktsizeB];
    toretbuffers[0] = repairbuf;
    decodeBufMap.insert(make_pair(0, (char*)repairbuf));

    // decode
    decodeTime -= getCurrentTime();
    if (systype == "native")    {
      rscode->decode(availbuffers, k, toretbuffers, 1, pktsizeB);
    } else {
      for (int taskid=0; taskid<decodetasks.size(); taskid++) {
        ECTask* compute = decodetasks[taskid];
	vector<int> children = compute->getChildren();
	unordered_map<int, vector<int>> coefMap = compute->getCoefMap();
	int col = children.size();
	int row = coefMap.size();
	vector<int> targets;
	int* matrix = (int*)calloc(row*col, sizeof(int));
	char** data = (char**)calloc(col, sizeof(char*));
	char** code = (char**)calloc(row, sizeof(char*));
	for (int bufIdx=0; bufIdx<children.size(); bufIdx++) {
          int child = children[bufIdx];
	  data[bufIdx] = decodeBufMap[child];
	}
	int codeBufIdx = 0;
	for (auto it: coefMap) {
          int target = it.first;
	  char* codebuf;
	  if (decodeBufMap.find(target) == decodeBufMap.end()) {
            codebuf = (char*)calloc(pktsizeB, sizeof(char));
	    decodeBufMap.insert(make_pair(target, codebuf));
	  } else {
            codebuf = decodeBufMap[target];
	  }
	  code[codeBufIdx] = codebuf;
	  targets.push_back(target);
	  vector<int> coef = it.second;
	  for (int j=0; j<col; j++) {
            matrix[codeBufIdx * col + j] = coef[j];
	  }
	  codeBufIdx++;
        }
	Computation::Multi(code, data, matrix, row, col, pktsizeB, "Isal");
	free(matrix);
	free(data);
	free(code);
      }
    }
    decodeTime += getCurrentTime();

    // check correcness
    bool success = true;
    for(int i=0; i<pktsizeB; i++) {
      if (oribuffer[i] != repairbuf[i]) {
        success = false;
	break;
      }
    }
    if (!success) {
      cout << "repair error!" << endl;
      break;
    }
  }
  cout << "============================================" << endl;
  cout << "InitEncodeTime: " << initEncodeTime/1000 << " ms" << endl;
  cout << "PureEncodeTime: " << encodeTime/1000 << " ms" << endl;
  cout << "PureEncodeThroughput: " << blocksizeB*k/1.048576/encodeTime << endl;
  cout << "InitDecodeTime: " << initDecodeTime/1000 << " ms" << endl;
  cout << "PureDecodeTime: " << decodeTime/1000 << " ms" << endl;
  cout << "PureDecodeThroughput: " << blocksizeB/1.048576/decodeTime << endl;


//  // take out original data
//  uint8_t* oribuf = (uint8_t*)calloc(pktsizeB, sizeof(uint8_t));
//  if (fidx<k) memcpy(oribuf, databuffers[fidx], pktsizeB);
//  else memcpy(oribuf, codebuffers[fidx-k], pktsizeB);
//
//  // prepare avail buffer and torec buffer
//  uint8_t** availbuffers = (uint8_t**)calloc(k, sizeof(uint8_t*));
//  uint8_t** toretbuffers = (uint8_t**)calloc(1, sizeof(uint8_t*));
//  int aidx=0;
//  for (int i=0; i<k && aidx<k; i++) {
//    if (i == fidx) continue;
//    availbuffers[aidx] = (uint8_t*)calloc(pktsizeB, sizeof(uint8_t));
//    memcpy(availbuffers[aidx], databuffers[i], pktsizeB);
//    aidx++;
//  }
//  for (int i=0; i<(n-k) && aidx<k; i++) {
//    if (i+k == fidx) continue;
//    availbuffers[aidx] = (uint8_t*)calloc(pktsizeB, sizeof(uint8_t));
//    memcpy(availbuffers[aidx], codebuffers[i], pktsizeB);
//    aidx++;
//  }
//  toretbuffers[0] = (uint8_t*)calloc(pktsizeB, sizeof(uint8_t));
//  memset(toretbuffers[0], 0, pktsizeB);

//  if (systype == "native") {
//    rscode->check(fidx);
//  } else {
//    for(int i=0; i<n; i++) {
//      if (i == fidx) torecidx.push_back(i);
//      else availidx.push_back(i);
//    }
//    for (int i=0; i<k; i++) {
//      int idx = availidx[i];
//      decodeBufMap.insert(make_pair(idx, (char*)availbuffers[i]));
//    }
//    decodeBufMap.insert(make_pair(fidx, (char*)toretbuffers[0]));
//    decdag = ec->Decode(availidx, torecidx);
//    vector<int> toposeq = decdag->toposort();
//    for (int i=0; i<toposeq.size(); i++) {
//      ECNode* curnode = decdag->getNode(toposeq[i]);
//      curnode->parseForClient(decodetasks);
//    }
//  }
//  initDecodeTime += getCurrentTime();

//  // perform decode
//  double decodeTime = 0;
//  decodeTime -= getCurrentTime();
//  for (int i=0; i<stripenum; i++) {
//    if (systype == "native")    {
//      rscode->decode(availbuffers, k, toretbuffers, 1, pktsizeB);
//      //rscode->decode2(availbuffers, k, toretbuffers, 1, pktsizeB, mat);
//    } else {
//      for (int taskid=0; taskid<decodetasks.size(); taskid++) {
//        ECTask* compute = decodetasks[taskid];
//	vector<int> children = compute->getChildren();
//	unordered_map<int, vector<int>> coefMap = compute->getCoefMap();
//	int col = children.size();
//	int row = coefMap.size();
//	vector<int> targets;
//	int* matrix = (int*)calloc(row*col, sizeof(int));
//	char** data = (char**)calloc(col, sizeof(char*));
//	char** code = (char**)calloc(row, sizeof(char*));
//	for (int bufIdx=0; bufIdx<children.size(); bufIdx++) {
//          int child = children[bufIdx];
//	  data[bufIdx] = decodeBufMap[child];
//	}
//	int codeBufIdx = 0;
//	for (auto it: coefMap) {
//          int target = it.first;
//	  char* codebuf;
//	  if (decodeBufMap.find(target) == decodeBufMap.end()) {
//            codebuf = (char*)calloc(pktsizeB, sizeof(char));
//	    decodeBufMap.insert(make_pair(target, codebuf));
//	  } else {
//            codebuf = decodeBufMap[target];
//	  }
//	  code[codeBufIdx] = codebuf;
//	  targets.push_back(target);
//	  vector<int> coef = it.second;
//	  for (int j=0; j<col; j++) {
//            matrix[codeBufIdx * col + j] = coef[j];
//	  }
//	  codeBufIdx++;
//        }
//	Computation::Multi(code, data, matrix, row, col, pktsizeB, "Isal");
//	free(matrix);
//	free(data);
//	free(code);
//      }
//    }
//  }
//  decodeTime += getCurrentTime();
//  overallDecodeTime += getCurrentTime();
//  cout << "--------------------------------------------" << endl;
//  cout << "OverallDecodeTime: " << overallDecodeTime/1000 << " ms" << endl;
//  cout << "InitDecodeTime: " << initDecodeTime/1000 << " ms" << endl;
//  cout << "PureDecodeTIme: " << decodeTime/1000 << " ms" << endl;
//  cout << "OverallDecodeThroughput: " << blocksizeB/1.048576/overallDecodeTime << endl;
//  cout << "PureDecodeThroughput: " << blocksizeB/1.048576/decodeTime << endl;
//
//  char* repairedbuf = (char*)toretbuffers[0];
//  bool repaired=true;
//  for (int i=0; i<pktsizeB; i++) {
//    if (oribuf[i] != repairedbuf[i]) {
//      repaired = false;
//      break;
//    }
//  }
//  if (repaired) {
//    cout << "repaired!" << endl;
//  } else {
//    cout << "repair error!" << endl;
//  }
//
//  // free data buffers
//  for (int i=0; i<k; i++) free(databuffers[i]);
//  free(databuffers);
//  for (int i=0; i<(n-k); i++) free(codebuffers[i]);
//  free(codebuffers);
//
//  free(oribuf);
//  for (int i=0; i<k; i++) free(availbuffers[i]);
//  free(availbuffers);
//  free(toretbuffers[0]);
//
//  if (rscode) delete rscode;
//  if (ec) delete ec;
//  if (encdag) delete encdag;
//  for (auto task: encodetasks) delete task;
//  encodeBufMap.clear();
}
