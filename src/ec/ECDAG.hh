#ifndef _ECDAG_HH_
#define _ECDAG_HH_

#include "../inc/include.hh"
#include "../protocol/AGCommand.hh"

#include "Cluster.hh"
#include "ECNode.hh"

using namespace std;

#define ECDAG_DEBUG_ENABLE true
#define BINDSTART 200
#define OPTSTART 300

class ECDAG {
  private:
    unordered_map<int, ECNode*> _ecNodeMap;
    vector<int> _ecHeaders;
    int _bindId = BINDSTART;
    vector<Cluster*> _clusterMap;
    int _optId = OPTSTART; 

    int findCluster(vector<int> childs);
  public:
    ECDAG(); 
    ~ECDAG();

    void Join(int pidx, vector<int> cidx, vector<int> coefs);
    int BindX(vector<int> idxs);
    void BindY(int pidx, int cidx);

    // topological sorting
    vector<int> toposort();
    ECNode* getNode(int cidx);
    vector<int> getHeaders();
    vector<int> getLeaves();

    // ecdag reconstruction
    void reconstruct(int opt);
    void optimize(int opt, 
                  unordered_map<int, pair<string, unsigned int>> objlist,
                  unordered_map<unsigned int, string> ip2Rack,
                  int ecn,
                  int eck,
                  int ecw);
    void optimize2(int opt, 
                  unordered_map<int, unsigned int>& cid2ip,
                  unordered_map<unsigned int, string> ip2Rack,
                  int ecn, int eck, int ecw,
                  unordered_map<int, unsigned int> sid2ip,
                  vector<unsigned int> allIps,
                  bool locality);
    void Opt0();
    void Opt1();
    void Opt2(unordered_map<int, string> n2Rack);

    // parse cmd
    unordered_map<int, AGCommand*> parseForOEC(unordered_map<int, unsigned int> cid2ip,
                                   string stripename, 
                                   int n, int k, int w, int num,
                                   unordered_map<int, pair<string, unsigned int>> objlist);
    vector<AGCommand*> persist(unordered_map<int, unsigned int> cid2ip, 
                                  string stripename,
                                  int n, int k, int w, int num,
                                  unordered_map<int, pair<string, unsigned int>> objlist);

    // for debug
    void dump();
};
#endif
