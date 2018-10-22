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
  
  // topological sorting
  vector<int> sortedList = ecdag->toposort();
  cout << "ECDAGTest::topological sorting: ";
  for (int i=0; i<sortedList.size(); i++) cout << sortedList[i] << " ";
  cout << endl;

  // create command for online encoding

  return 0;
}
