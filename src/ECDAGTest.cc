#include "ec/ECBase.hh"
#include "ec/ECDAG.hh"
#include "ec/RSCONV.hh"  // should delete later
#include "common/Config.hh"
#include "inc/include.hh"

using namespace std;

void usage() {
  cout << "Usage: ./ECDAGTest parsetype code operation" << endl;
  cout << "  0. parsetype (online/offline)" << endl;
  cout << "  1. code (rs_9_6_op1/waslrc/ia/drc643/rsppr/drc963/rawrs/clay_6_4/butterfly_6_4)" << endl;
  cout << "  2. operation (encode/decode)" << endl;
}

int loss(int ecn) {
  // simulate a loss index
  return 0;
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

  // get ecpolicy
  ECPolicy* ecpolicy = conf->_ecPolicyMap[ecid];
  ECBase* ec = ecpolicy->createECClass();
  int ecn = ecpolicy->getN();
  int eck = ecpolicy->getK();
  int ecw = ecpolicy->getW();
  int opt = ecpolicy->getOpt();
  bool locality = ecpolicy->getLocality();
  cout << "ecn: " << ecn << ", eck: " << eck << ", ecw: " << ecw << ", opt: " << opt << endl;

  // create ECDAG
  ECDAG* ecdag;
  if (operation == "encode") {
    ecdag = ec->Encode();
  } else if (operation == "decode") {
    int lostidx = loss(ecn);
    vector<int> availcidx;
    vector<int> toreccidx;
    for (int i=0; i<ecn; i++) {
      if (i == lostidx) {
        for (int j=0; j<ecw; j++) {
          toreccidx.push_back(i*ecw+j);
        }
      } else {
        for (int j=0; j<ecw; j++) {
          availcidx.push_back(i*ecw+j);
        }
      }
    }
    ecdag = ec->Decode(availcidx, toreccidx);
  } else {
    cout << "Unrecognized operation" << endl;
    return -1;
  }

  if (parsetype == "online") {
    ecdag->dump();
    // topological sorting
    vector<int> toposeq = ecdag->toposort();
    cout << "toposort: ";
    for (int i=0; i<toposeq.size(); i++) cout << toposeq[i] << " ";
    cout << endl;

    vector<ECTask*> computetasks;
    for (int i=0; i<toposeq.size(); i++) {
      ECNode* curnode = ecdag->getNode(toposeq[i]);
      curnode->parseForClient(computetasks);
    }
    for (int i=0; i<computetasks.size(); i++) computetasks[i]->dump();
  } else {

    // first optimize without physical information
    ecdag->reconstruct(opt);

    // simulate physical information
    unordered_map<int, pair<string, unsigned int>> objlist;
    unordered_map<int, unsigned int> sid2ip;
    for (int sid=0; sid<ecn; sid++) {
      string objname = "testobj"+to_string(sid);
      unsigned int ip = conf->_agentsIPs[sid];
      objlist.insert(make_pair(sid, make_pair(objname, ip)));
      sid2ip.insert(make_pair(sid, ip));
    }

    // topological sorting
    vector<int> toposeq = ecdag->toposort();
    cout << "toposort: ";
    for (int i=0; i<toposeq.size(); i++) cout << toposeq[i] << " ";
    cout << endl;

    // cid2ip
    unordered_map<int, unsigned int> cid2ip;
    for (int i=0; i<toposeq.size(); i++) {
      int curcid = toposeq[i];
      ECNode* cnode = ecdag->getNode(curcid);
      vector<unsigned int> candidates = cnode->candidateIps(sid2ip, cid2ip, conf->_agentsIPs, ecn, eck, ecw, locality || (opt>0));
      unsigned int ip = candidates[0];
      cid2ip.insert(make_pair(curcid, ip));
    }
    
    ecdag->optimize2(opt, cid2ip, conf->_ip2Rack, ecn, eck, ecw, sid2ip, conf->_agentsIPs, locality || (opt>0));
    ecdag->dump();

    for (auto item: cid2ip) {
      cout << "cid: " << item.first << ", ip: " << RedisUtil::ip2Str(item.second) << ", ";
      ECNode* cnode = ecdag->getNode(item.first);
      unordered_map<int, int> map = cnode->getRefMap();
      for (auto iitem: map) {
        cout << "ref["<<iitem.first<<"]: " << iitem.second << ", ";
      }
      cout << endl;
    }

    string stripename = "teststripe";
    int pktnum = 8;
    unordered_map<int, AGCommand*> agCmds = ecdag->parseForOEC(cid2ip, stripename, ecn, eck, ecw, pktnum, objlist);
    vector<AGCommand*> persistCmds = ecdag->persist(cid2ip, stripename, ecn, eck, ecw, pktnum, objlist);
  }

  return 0;
}
