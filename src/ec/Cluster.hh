#ifndef _CLUSTER_HH_
#define _CLUSTER_HH_

#include "../inc/include.hh"

#define CLUSTER_DEBUG_ENABLE false

using namespace std;

class Cluster {
  private:
    vector<int> _childs;
    vector<int> _parents;
    
    int _opted;

  public:
    Cluster(vector<int> childs, int parent);
    bool childsInCluster(vector<int> childs);
    void addParent(int parent);
//    void setChilds(vector<int> childs);
    void setOpt(int opt);
//    void reset(vector<int> childs, vector<int> parents);

    int getOpt();
    vector<int> getParents();
    vector<int> getChilds();
    void dump();
};

#endif 
