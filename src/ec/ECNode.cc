#include "ECNode.hh"

ECNode::ECNode(int id) {
  _nodeId = id;
  _hasConstraint = false;
  _consId = -1;
}

ECNode::~ECNode() {
  for (auto item: _oecTasks) {
    delete item.second;
  }
  _oecTasks.clear();
}

void ECNode::addCoefs(int calfor, vector<int> coefs) {
  if (_coefMap.find(calfor) != _coefMap.end()) {
    _coefMap[calfor].clear();
    _coefMap[calfor] = coefs;
  } else {
    _coefMap.insert(make_pair(calfor, coefs));
  }
}

void ECNode::cleanChilds() {
  _childNodes.clear();
  _coefMap.clear();
}

void ECNode::setChilds(vector<ECNode*> childs) {
  _childNodes = childs; 
}

int ECNode::getChildNum() {
  return _childNodes.size();
}

vector<ECNode*> ECNode::getChildren() {
  return _childNodes;
}

ECNode* ECNode::getChildNode(int cid) {
  for (auto item: _childNodes) {
    if (item->getNodeId() == cid) return item;
  }
}

void ECNode::incRefNumFor(int id) {
  if (_refNumFor.find(id) == _refNumFor.end()) _refNumFor.insert(make_pair(id, 1));
  else _refNumFor[id]++;

  // if current node is lined to a bind node, we also need to update bind node
  if (_childNodes.size() == 1 && _childNodes[0]->getCoefmap().size() > 1) {
    _childNodes[0]->incRefNumFor(id);
  }
}

void ECNode::decRefNumFor(int id) {
  assert (_refNumFor.find(id) != _refNumFor.end());
  _refNumFor[id]--;

  // if current node is linked to a bind node, we also need to update bind node
  if (_childNodes.size() == 1 && _childNodes[0]->getCoefmap().size() > 1) {
    _childNodes[0]->decRefNumFor(id);
  }
}

void ECNode::cleanRefNumFor(int id) {
  assert (_refNumFor.find(id) != _refNumFor.end());
  _refNumFor[id] = 0;
  // TODO: is there any chain?
}

int ECNode::getRefNumFor(int id) {
//  assert (_refNumFor.find(id) != _refNumFor.end());
  if (_refNumFor.find(id) != _refNumFor.end()) return _refNumFor[id];
  else return 0;
//  return _refNumFor[id];
}

void ECNode::setRefNum(int nid, int ref) {
  _refNumFor[nid] = ref;

  // if current node is lined to a bind node, we also need to update bind node
  if (_childNodes.size() == 1 && _childNodes[0]->getCoefmap().size() > 1) {
    _childNodes[0]->setRefNum(nid, ref);
  }
}

unordered_map<int, int> ECNode::getRefMap() {
  return _refNumFor;
}

int ECNode::getNodeId() {
  return _nodeId;
}

unordered_map<int, vector<int>> ECNode::getCoefmap() {
  return _coefMap;
}

int ECNode::getCoefOfChildForParent(int child, int parent) {
  assert (_coefMap.find(parent) != _coefMap.end());
  vector<int> coefs = _coefMap[parent];
  
  for (int i=0; i<_childNodes.size(); i++) {
    if (_childNodes[i]->_nodeId == child) return coefs[i];
  }

  return -1;
}

void ECNode::setConstraint(bool cons, int id) {
  _hasConstraint = cons;
  if (_hasConstraint) _consId = id;
}

void ECNode::dump(int parent) {
  if (parent == -1) parent = _nodeId;
  cout << "(data" << _nodeId;
  if (_childNodes.size() > 0) {
    cout << " = ";
  }
  vector<int> curCoef;
  if (_coefMap.size() > 1) {
    unordered_map<int, vector<int>>::const_iterator c = _coefMap.find(parent);
    assert (c!=_coefMap.end());
    curCoef = _coefMap[parent];
  } else if (_coefMap.size() == 1) {
    unordered_map<int, vector<int>>::const_iterator c = _coefMap.find(_nodeId);
    assert (c!=_coefMap.end());
    curCoef = _coefMap[_nodeId];
  }
  for (int i=0; i<_childNodes.size(); i++) {
    cout << curCoef[i] << " ";
    _childNodes[i]->dump(_nodeId);
    if (i < _childNodes.size() - 1) {
      cout << " + ";
    }
  }
  cout << ")";
}

void ECNode::parseForClient(vector<ECTask*>& tasks) {
  // we parse compute task for client
  bool computebool = false;
  // if the number of children is larger than 1, there must be calculation
  if (_childNodes.size() > 1) computebool = true;
  else if (_childNodes.size() == 0) computebool = false;
  else {
    // if the number of children equals to 1, 
    ECNode* childNode = _childNodes[0];
    // 1> this node is linked to a bind node, there is no calculation
    if (childNode->getCoefmap().size() > 1) computebool = false;
    // 2> this node is not linked to a bind node, there is calculation of res = coef * value
    else if (childNode->getCoefmap().size() == 1) computebool = true;
  }

  if (computebool) {
    ECTask* compute = new ECTask();
    compute->setType(2);
    vector<int> children;
    for (int i=0; i<_childNodes.size(); i++) children.push_back(_childNodes[i]->getNodeId());
    compute->setChildren(children);
    compute->setCoefmap(_coefMap);
    tasks.push_back(compute);
  }
}

vector<unsigned int> ECNode::candidateIps(unordered_map<int, unsigned int> sid2ip,
                                          unordered_map<int, unsigned int> cid2ip,
                                          vector<unsigned int> allIps,
                                          int n,
                                          int k,
                                          int w,
                                          bool locality) {
  vector<unsigned int> toret;
  int sid = _nodeId/w;

  // 0. current node has constraint
  if (_hasConstraint) {
    assert(cid2ip.find(_consId) != cid2ip.end());
    toret.push_back(cid2ip[_consId]);
    return toret;
  }

  // 1. current node is preassigned a location
  if (sid2ip.find(sid) != sid2ip.end()) {
    toret.push_back(sid2ip[sid]);
    return toret;
  }

  // 2. if locality is enabled prepare candidate from children
  if (locality) {
    for (int i=0; i<_childNodes.size(); i++) {
      int cidx = _childNodes[i]->getNodeId();
      assert (cid2ip.find(cidx) != cid2ip.end());
      toret.push_back(cid2ip[cidx]);
    }
    return toret;
  } else {
    // prepare candidate without child
    vector<unsigned int> childIps;
    for (int i=0; i<_childNodes.size(); i++) {
      int cidx = _childNodes[i]->getNodeId();
      assert (cid2ip.find(cidx) != cid2ip.end());
      childIps.push_back(cid2ip[cidx]);
    }
    for (auto ip: allIps) {
      if (find(childIps.begin(), childIps.end(), ip) == childIps.end())
        toret.push_back(ip);
    }
    if (toret.size() == 0) {
      // choose randomly
      srand((unsigned)time(0));
      int randomidx = rand() % childIps.size();
      toret.push_back(childIps[randomidx]);
    }
    return toret;
  }
}

vector<unsigned int> ECNode::candidateIps(unordered_map<int, unsigned int> sid2ip,
                                          unordered_map<int, unsigned int> cid2ip,
                                          vector<unsigned int> allIps,
                                          int n,
                                          int k,
                                          int w,
                                          bool locality, int lostid) {
  vector<unsigned int> toret;
  int sid = _nodeId/w;

  // 0. current node has constraint
  if (_hasConstraint) {
    assert(cid2ip.find(_consId) != cid2ip.end());
    toret.push_back(cid2ip[_consId]);
    return toret;
  }

  // 1. current node is preassigned a location
  if (sid2ip.find(sid) != sid2ip.end() && sid != lostid) {
    toret.push_back(sid2ip[sid]);
    return toret;
  }

  // 2. if locality is enabled prepare candidate from children
  if (locality) {
    for (int i=0; i<_childNodes.size(); i++) {
      int cidx = _childNodes[i]->getNodeId();
      assert (cid2ip.find(cidx) != cid2ip.end());
      toret.push_back(cid2ip[cidx]);
    }
    return toret;
  } else {
    // prepare candidate without child
    vector<unsigned int> childIps;
    for (int i=0; i<_childNodes.size(); i++) {
      int cidx = _childNodes[i]->getNodeId();
      assert (cid2ip.find(cidx) != cid2ip.end());
      childIps.push_back(cid2ip[cidx]);
    }
    for (auto ip: allIps) {
      if (find(childIps.begin(), childIps.end(), ip) == childIps.end())
        toret.push_back(ip);
    }
    if (toret.size() == 0) {
      // choose randomly
      srand((unsigned)time(0));
      int randomidx = rand() % childIps.size();
      toret.push_back(childIps[randomidx]);
    }
    return toret;
  }
}

void ECNode::parseForOEC(unsigned int ip) {
  _ip = ip;
  bool cache = true;
  // 0. check for Load
  // case: When current node is leaf, there is Load Task
  int childNum = _childNodes.size();
  if (childNum == 0) {
    ECTask* load = new ECTask(); 
    load->setType(0);
    load->addIdx(_nodeId);
    _oecTasks.insert(make_pair(0, load));
  }

  // 1&2 check for Fetch and Compute
  // We check for Compute and Fetch together.
  // If there is ComputeTask, there must be FetchTask
  if (childNum > 1) {
    // there is more than 1 child for computation
    vector<int> childrenIdx;
    for (int i=0; i<childNum; i++) {
      childrenIdx.push_back(_childNodes[i]->getNodeId());
    } 
    // 1. fetch
    ECTask* fetch = new ECTask();
    fetch->setType(1);
    fetch->setChildren(childrenIdx);
    _oecTasks.insert(make_pair(1, fetch));

    // 2. compute
    ECTask* compute = new ECTask();
    compute->setType(2);
    compute->setChildren(childrenIdx);
    compute->setCoefmap(_coefMap);
    _oecTasks.insert(make_pair(2, compute));
    
  } else if (childNum == 1) {
    // case 1: child is bindnode, then there is no computation or fetch
    // case 2: there is computation to transfer 1 id to another (e.g. the leaf in ClayCode)
    ECNode* childnode = _childNodes[0];
    unordered_map<int, vector<int>> cmap = childnode->getCoefmap();
    int childtarget = cmap.size();
    if (childtarget > 1) {
      // child is bindnode there is no need to create compute and fetch task 
      // however, this node need to tell where should fetch data
      // create a tell task
      //ECTask* tell = new ECTask();
      //tell->setType(5);
      //tell->setBind(childnode->getNodeId());
      //_oecTasks.insert(make_pair(5, tell));
      _ip = childnode->getIp();  // ?? Not sure
      cache = false;
    } else {
      // child node is not bind node
      vector<int> childrenIdx;
      childrenIdx.push_back(childnode->getNodeId());

      // 1. fetch
      ECTask* fetch = new ECTask();
      fetch->setType(1);
      fetch->setChildren(childrenIdx);
      _oecTasks.insert(make_pair(1, fetch));

      // 2. compute
      ECTask* compute = new ECTask();
      compute->setType(2);
      compute->setChildren(childrenIdx);
      compute->setCoefmap(_coefMap);
      _oecTasks.insert(make_pair(2, compute));
    }
  }

  // 3. check for Cache
  // Basically, we need to cache the result of Load or Compute.
//  int persistDSS = 0;
//  if (_refNumFor.size() == 0) persistDSS = 1;
//  if (persistDSS == 1) _refNumFor.insert(make_pair(_nodeId, 1));

//  if (_refNumFor.size() == 0) _refNumFor.insert(make_pair(_nodeId, 1));
  if (cache) {
    ECTask* cache = new ECTask();
    cache->setType(3);
    cache->addRef(_refNumFor);
    _oecTasks.insert(make_pair(3, cache));
  }
}

unordered_map<int, ECTask*> ECNode::getTasks() {
  return _oecTasks;
}

void ECNode::clearTasks() {
  for (auto item: _oecTasks) {
    delete item.second;
  }
  _oecTasks.clear();
}

unsigned int ECNode::getIp() {
  return _ip;
}

AGCommand* ECNode::parseAGCommand(string stripename,
                                  int n, int k, int w,
                                  int num,
                                  unordered_map<int, pair<string, unsigned int>> stripeobjs,
                                  unordered_map<int, unsigned int> cid2ip) {
  // type 2: load & cache
  // type 3: fetch & compute & cache
  // type 4: tells ip
  // type 5: fetch & cache
  // type 7: read & fetch & compute & cache
  bool load = false;
  bool fetch = false;
  bool compute = false;
  bool cache = false;

  if (_oecTasks.find(0) != _oecTasks.end()) load = true;
  if (_oecTasks.find(1) != _oecTasks.end()) fetch = true;
  if (_oecTasks.find(2) != _oecTasks.end()) compute = true;
  if (_oecTasks.find(3) != _oecTasks.end()) cache = true;
//  cout << "ECNode::parseAGCommand.load: " << load << ", fetch: " << fetch << ", compute: " << compute << ", cache:" << cache << endl;
 
  if (load & !fetch & !compute & cache) {
    // load from disk
    vector<int> indices = _oecTasks[0]->getIndices();
    int sid = indices[0]/w;
    pair<string, unsigned int> curpair = stripeobjs[sid]; 
    string objname = curpair.first;

    AGCommand* agCmd = new AGCommand();
    agCmd->buildType2(2, _ip, stripename, w, num, objname, indices, _oecTasks[3]->getRefMap());
    return agCmd;
  }

  if (!load & fetch & compute & cache) {
    // from fetch
    vector<int> prevCids = _oecTasks[1]->getChildren();
    vector<unsigned int> prevLocs;
    for (int i=0; i<prevCids.size(); i++) {
      int cidx = prevCids[i];
      //unsigned int ip = cid2ip[cidx];
      ECNode* cnode = getChildNode(cidx);
      unsigned int ip = cnode->getIp();
      prevLocs.push_back(ip);
    }
    unordered_map<int, vector<int>> coefs = _oecTasks[2]->getCoefMap();
    unordered_map<int, int> refs = _oecTasks[3]->getRefMap();
    // fetch and compute
    AGCommand* agCmd = new AGCommand();
    agCmd->buildType3(3, _ip, stripename, w, num, prevCids.size(), prevCids, prevLocs, coefs, refs);
    return agCmd;
  }

  return NULL;
}

void ECNode::dumpRawTask() {
  cout << "Raw ECTasks : " << _nodeId << ", " << RedisUtil::ip2Str(_ip) << endl;
  for (int i=0; i<4; i++) {
    if (_oecTasks.find(i) == _oecTasks.end()) continue;
    ECTask* ectask = _oecTasks[i];
    if (i == 0) {
      vector<int> indices = ectask->getIndices();
      cout << "    Load: " ;
      for (int j=0; j<indices.size(); j++) cout << indices[j] << " ";
      cout << endl;
    } else if (i == 1) {
      cout << "    Fetch: ( ";
      vector<int> childrenIdx = ectask->getChildren();
      for (int j=0; j<childrenIdx.size(); j++) cout << childrenIdx[j] << " ";
      cout << ")" << endl;
    } else if (i == 2) {
      vector<int> childrenIdx = ectask->getChildren();
      unordered_map<int, vector<int>> coefmap = ectask->getCoefMap();
      for (auto item: coefmap) {
        int target = item.first;
        vector<int> coefs = item.second;
        cout << "    Compute: " << target << " = ( ";
        for (int j=0; j<childrenIdx.size(); j++) cout << childrenIdx[j] << " ";
        cout << ") * ( ";
        for (int j=0; j<coefs.size(); j++) cout << coefs[j] << " ";
        cout << ")" << endl;
      }
    } else if (i == 3) {
      unordered_map<int, int> refmap = ectask->getRefMap();
      int persistDSS = ectask->getPersistType();
      for (auto item: refmap) {
        int target = item.first;
        int ref = item.second;
        cout << "    Cache: ";
        cout << target << ": " << ref << " times " << endl;
      }
    }
  }
}

