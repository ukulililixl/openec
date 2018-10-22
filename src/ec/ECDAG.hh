#ifndef _ECDAG_HH_
#define _ECDAG_HH_

#include "../inc/include.hh"

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

    void Join(int pidx, vector<int> cidx, vector<int> coefs);
    int BindX(vector<int> idxs);
    void BindY(int pidx, int cidx);

    // topological sorting
    vector<int> toposort();

    // for debug
    void dump();
};
#endif
