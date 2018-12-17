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

  // this is used for native impl
  NativeRS* rscode = nullptr;
   
  // this is used for openec impl
  ECPolicy* ecpolicy = nullptr;
  ECBase* ec = nullptr;
  ECDAG* ecdag = nullptr;

  double overallEncodeTime=0, initEncodeTime=0;
  overallEncodeTime -= getCurrentTime();
  initEncodeTime -= getCurrentTime();

  if (systype == "native") {
    rscode = new NativeRS();
  } else {
    //string ecid = "rs_"+to_string(n)+"_"+to_string(k)+"_bindx";
    string ecid = "rs_"+to_string(n)+"_"+to_string(k);
    cout << ecid << endl;
    ecpolicy = conf->_ecPolicyMap[ecid];
    ec = ecpolicy->createECClass();
  }

  // initialize
  // native: create matrix
  // openec: create ecdag and parse ecdag
  vector<ECTask*> encodetasks;
  if (systype == "native") {
    rscode->initialize(n, k);
  } else {
    ecdag = ec->Encode();
    vector<int> toposeq = ecdag->toposort();
    for (int i=0; i<toposeq.size(); i++) {
      ECNode* curnode = ecdag->getNode(toposeq[i]);
      curnode->parseForClient(encodetasks);
    }
  }
  initEncodeTime += getCurrentTime();

  // prepare data buffer
  uint8_t** databuffers = (uint8_t**)calloc(k, sizeof(uint8_t*));
  for (int i=0; i<k; i++) {
    databuffers[i] = (uint8_t*)calloc(pktsizeB, sizeof(uint8_t));
    uint8_t initv = (uint8_t)i;
    memset(databuffers[i], initv, pktsizeB);
  }
  uint8_t** codebuffers = (uint8_t**)calloc((n-k), sizeof(uint8_t*));
  for (int i=0; i<(n-k); i++) {
    codebuffers[i] = (uint8_t*)calloc(pktsizeB, sizeof(uint8_t));
    memset(codebuffers[i], 0, pktsizeB);
  }

  unordered_map<int, char*> bufMap;
  if (systype == "openec") {
    for (int i=0; i<k; i++) bufMap.insert(make_pair(i, (char*)databuffers[i]));
    for (int i=0; i<(n-k); i++) bufMap.insert(make_pair(k+i, (char*)codebuffers[i]));
  }

  // encode
  int stripenum = blocksizeB/pktsizeB;
  double encodeTime = 0;
  for (int i=0; i<stripenum; i++) {
    encodeTime -= getCurrentTime(); 
    if (systype == "native") {
      rscode->construct(databuffers, codebuffers, pktsizeB);
    } else {
      // perform computation in encodetasks one by one
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
        for (int bufIdx = 0; bufIdx < children.size(); bufIdx++) {
	  int child = children[bufIdx];
	  data[bufIdx] = bufMap[child];
	}
	int codeBufIdx = 0;
	for (auto it: coefMap) {
	  int target = it.first;
	  char* codebuf;
	  if (bufMap.find(target) == bufMap.end()) {
	    codebuf = (char*)calloc(pktsizeB, sizeof(char));
	    bufMap.insert(make_pair(target, codebuf));
	  } else {
	    codebuf = bufMap[target];
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
      }
    }
    encodeTime += getCurrentTime();
  }
  overallEncodeTime += getCurrentTime();
  cout << "=============================" << endl;
  cout << "InitEncodeTime: " << initEncodeTime/1000 << " ms" << endl;
  cout << "EncodeTime: " << encodeTime/1000 << " ms" << endl;
  cout << "OverallEncodeTime: " << overallEncodeTime/1000 << " ms" << endl;
  cout << "EncodeThroughput: (MB/s): " << blocksizeB*k/1.048576 / encodeTime << endl;
  cout << "OverallThroughput: (MB/s): " << blocksizeB*k/1.048576 / overallEncodeTime << endl;

  // clean encode
  if (systype == "openec") {
    delete ecdag;
    for (auto item: encodetasks) delete item;
  }
  cout << "-----------------------------" << endl;

  // simulate failure
  int fidx = 0;

  double overallDecodeTime = 0, initDecodeTime = 0;
  overallDecodeTime -= getCurrentTime();
  initDecodeTime -= getCurrentTime();

  // take out fidx buffer
  uint8_t* oribuf = (uint8_t*)calloc(pktsizeB, sizeof(uint8_t));
  if (fidx < k) memcpy(oribuf, databuffers[fidx], pktsizeB);
  else memcpy(oribuf, codebuffers[fidx-k], pktsizeB);
  // prepare torec buf for native code
  uint8_t** availbuffers = (uint8_t**)calloc(k, sizeof(uint8_t*));
  uint8_t** toretbuffers = (uint8_t**)calloc(1, sizeof(uint8_t*));
  int icount=0;
  for (int i=0; i<k && icount<k; i++) {
    if (i!=fidx) availbuffers[icount++] = databuffers[i];
  }
  for (int i=0; i<(n-k) && icount<k; i++) {
    if ((k+i)!=fidx) availbuffers[icount++] = codebuffers[i];
    if (icount >= k) break;
  }
  toretbuffers[0] = (uint8_t*)calloc(pktsizeB, sizeof(uint8_t));
  memset(toretbuffers[0], 0, pktsizeB);

  unordered_map<int, char*> decBufMap;
  if (systype == "openec") {
    for (int i=0; i<n; i++) {
      if (i == fidx) decBufMap.insert(make_pair(i, (char*)toretbuffers[0]));
      else {
        if (i<k) decBufMap.insert(make_pair(i, (char*)databuffers[i]));
	else decBufMap.insert(make_pair(i, (char*)codebuffers[i-k]));
      }
    }
  }

  // prepare indices for openec
  vector<int> availIdx;
  vector<int> torecIdx;
  vector<ECTask*> decodetasks;
  for (int i=0; i<n; i++) {
    if (i == fidx) torecIdx.push_back(i);
    else availIdx.push_back(i);
  }

  if (systype == "native") {
    rscode->check(fidx);
  } else {
    ecdag = ec->Decode(availIdx, torecIdx);
    vector<int> toposeq = ecdag->toposort();
    for (int i=0; i<toposeq.size(); i++) {
      ECNode* curnode = ecdag->getNode(toposeq[i]);
      curnode->parseForClient(decodetasks);
    }
  }
  initDecodeTime += getCurrentTime();

  // decode
  double decodeTime=0;
  for (int i=0; i<stripenum; i++) {
    decodeTime -= getCurrentTime();
    if (systype == "native") {
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
	// prepare the data buf
	// actually, data buf should always exist
	for (int bufIdx = 0; bufIdx < children.size(); bufIdx++) {
	  int child = children[bufIdx];
	  assert (decBufMap.find(child) != decBufMap.end());
	  data[bufIdx] = decBufMap[child];
	}
	// prepare the code buf
	int codeBufIdx = 0;
	for (auto it: coefMap) {
	  int target = it.first;
	  char* codebuf;
	  if (decBufMap.find(target) == decBufMap.end()) {
	    codebuf = (char*)calloc(pktsizeB, sizeof(char));
	    decBufMap.insert(make_pair(target, codebuf));
	  } else {
	    codebuf = decBufMap[target];
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
      }
    }
    decodeTime += getCurrentTime();
  }
  overallDecodeTime += getCurrentTime();
  cout << "InitDecodeTime: " << initDecodeTime/1000 << " ms" << endl;
  cout << "DecodeTime: " <<  decodeTime/1000 << " ms" << endl;
  cout << "OverallDecodeTime: " << overallDecodeTime/1000 << " ms" << endl;
  cout << "DecodeThroughput: " << blocksizeB/1.048576/decodeTime << endl;
  cout << "OverallDecodeThroughput: " << blocksizeB/1.048576/overallDecodeTime << endl;
  
  // free
  for (int i=0; i<k; i++) free(databuffers[i]);
  free(databuffers);
  for (int i=0; i<(n-k); i++) free(codebuffers[i]);
  free(codebuffers);
   
  if (systype == "native") delete rscode;
  else {
    delete ecdag;
    delete ec;
  }

  delete conf;
}
