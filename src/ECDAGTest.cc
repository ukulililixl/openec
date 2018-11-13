#include "ec/ECBase.hh"
#include "ec/ECDAG.hh"
#include "ec/RSCONV.hh"  // should delete later
#include "common/Config.hh"
#include "inc/include.hh"

using namespace std;

void usage() {
  cout << "Usage: ./ECDAGTest parsetype code operation" << endl;
  cout << "  0. parsetype (online/offline)" << endl;
  cout << "  1. code (rs/waslrc/ia/drc643/rsppr/drc963/rawrs/clay_6_4/butterfly_6_4)" << endl;
  cout << "  2. operation (encode/decode)" << endl;
}

int main(int argc, char** argv) {
 
  if (argc < 4)  {
    usage();
    exit(1);
  }

  string parsetype = string(argv[1]);
  string ecid = string(argv[2]);
  string operation = string(argv[3]);
  cout << "parsetype: " << parsetype << ", ecid: " << ecid << ", operation: " << operation << endl;

  string confpath = "conf/sysSetting.xml";
  Config* conf = new Config(confpath);

  // just for test
  int n=6, k=4, w=1;
  bool locality=false;
  int opt=-1;
  vector<string> param;
  ECBase* ec = new RSCONV(n, k, 1, locality, opt, param);

  // construct encode/decode ECDAG
  ECDAG* ecdag = ec->Encode(); 
  ecdag->dump();

  // simulate physical information for current stripe
  unordered_map<int, pair<string, unsigned int>> objlist;
  unordered_map<int, unsigned int> sid2ip;
  for (int i=0; i<n; i++) {
    string objname = "testobj-"+to_string(i);
    unsigned int ip = conf->_agentsIPs[i];
    pair<string, unsigned int> curpair = make_pair(objname, ip);
    objlist.insert(make_pair(i, curpair));
    sid2ip.insert(make_pair(i, ip));
    cout << "ECDAG:: sidx: " << i << ", objname: " << objname << ", loc: " << RedisUtil::ip2Str(ip) << endl;
  }

  // reconstruction/optimization is performed here
  // opt0: BindX by default
  // opt1: BindY
  // opt2: layering or pipelining reconstruction with physical information
  
  // topological sorting
  vector<int> sortedList = ecdag->toposort();
  cout << "ECDAGTest::topological sorting: ";
  for (int i=0; i<sortedList.size(); i++) cout << sortedList[i] << " ";
  cout << endl;

  // figure out corresponding ip for each compute node
  unordered_map<int, unsigned int> cid2ip;
  for (int i=0; i<sortedList.size(); i++) {
    int cidx = sortedList[i];
    ECNode* node = ecdag->getNode(cidx);
    vector<unsigned int> candidates = node->candidateIps(sid2ip, cid2ip, conf->_agentsIPs, n, k, w, true);
    // choose from candidates
    unsigned int curip = candidates[0];
    cid2ip.insert(make_pair(cidx, curip));
    cout << "ECDAGTest:: cidx: " << cidx << ", ip: " << RedisUtil::ip2Str(curip) << endl;
  }

  // preassign ip for root?

  // parse for OEC
  for (int i=0; i<sortedList.size(); i++) {
    int cidx = sortedList[i];
    ECNode* node = ecdag->getNode(cidx);
    unsigned int ip = cid2ip[cidx];
    node->parseForOEC(ip);
    node->dumpRawTask();
  }

  // create commands?
  
  // refine ectasks?
  // type0: no refine
  // type1: aggregate load tasks for sub-packetization
  // type2: aggregate load and compute
  


  

  return 0;
}
