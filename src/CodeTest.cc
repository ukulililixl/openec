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
  cout << "	2, operation (encode/decode)" << endl;
  cout << "	3, n" << endl;
  cout << "	4, k" << endl;
  cout << "	5, blocksizeB" << endl;
  cout << "	6, pktsizeB" << endl;
}

double getCurrentTime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec * 1e+6 + (double)tv.tv_usec;
}

int main(int argc, char** argv) {
  if (argc < 7) {
    usage();
    return 0;
  }

  string systype = string(argv[1]);
  string operation = string(argv[2]);
  int n = atoi(argv[3]);
  int k = atoi(argv[4]);
  int blocksizeB = atoi(argv[5]);
  int pktsizeB = atoi(argv[6]);

  string confpath = "conf/sysSetting.xml";
  Config* conf = new Config(confpath);

  NativeRS* rscode = nullptr;
  ECPolicy* ecpolicy = nullptr;
  ECBase* ec = nullptr;
  ECDAG* ecdag = nullptr;

  if (systype == "native") {
    rscode = new NativeRS();
  } else {
    string ecid = "rs_"+to_string(n)+"_"+to_string(k);
    cout << ecid << endl;
    ecpolicy = conf->_ecPolicyMap[ecid];
    ec = ecpolicy->createECClass();
  }

  // initialize
  // native: create matrix
  // openec: create ecdag and parse ecdag
  vector<ECTask*> computetasks;
  if (systype == "native") {
    rscode->initialize(n, k);
  } else {
    ecdag = ec->Encode();
    vector<int> toposeq = ecdag->toposort();
    for (int i=0; i<toposeq.size(); i++) {
      ECNode* curnode = ecdag->getNode(toposeq[i]);
      curnode->parseForClient(computetasks);
    }
  }

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
      // perform computation in computetasks one by one
      for (int taskid = 0; taskid < computetasks.size(); taskid++) {
        ECTask* compute = computetasks[taskid];
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
  cout << "Encode throughput: (MB/s): " << blocksizeB*k/1.048576 / encodeTime << endl;
  
  // free
  for (int i=0; i<k; i++) free(databuffers[i]);
  free(databuffers);
  for (int i=0; i<(n-k); i++) free(codebuffers[i]);
  free(codebuffers);
   
  if (systype == "native") delete rscode;
}
