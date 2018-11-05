#ifndef _ECNODE_HH_
#define _ECNODE_HH_

#include "ECTask.hh"
#include "../inc/include.hh"

#define ECNODE_DEBUG false
using namespace std;

class ECNode {
  private: 
    int _nodeId;

    // _childNodes record all children of current node
    vector<ECNode*> _childNodes;
    // For each <key, value> pair in _coefMap:
    // key: target _nodeId that we want to compute
    // value: corresponding coefs to compute key
    unordered_map<int, vector<int>> _coefMap;

    // _refNo records the number of parents of current node
    unordered_map<int, int> _refNumFor;

    bool _hasConstraint;
    int _consId; 

  public:
    ECNode(int id);
    ~ECNode();

    int getNodeId();

    void cleanChilds();
    void setChilds(vector<ECNode*> childs);
    int getChildNum();
    vector<ECNode*> getChildren();

    void addCoefs(int calfor, vector<int> coefs);
    unordered_map<int, vector<int>> getCoefmap();

    void incRefNumFor(int id); // increase refnum for id
    int getRefNumFor(int id);

    // parseForClient compute tasks
    void parseForClient(vector<ECTask*>& tasks);

    // for debug
    void dump(int parent);
};

#endif
