#include "ECDAG.hh"

ECDAG::ECDAG() {
}

ECDAG::~ECDAG() {
  for (auto it: _ecNodeMap) {
    if (it.second) {
      delete it.second;
      it.second = nullptr;
    }
  }
  _ecNodeMap.clear();

  for (auto it: _clusterMap) delete it;
}

int ECDAG::findCluster(vector<int> childs) {
  for (int i=0; i<_clusterMap.size(); i++) {
    if (_clusterMap[i]->childsInCluster(childs)) return i;
  }
  return -1;
}

void ECDAG::Join(int pidx, vector<int> cidx, vector<int> coefs) {
  // debug start
  string msg = "ECDAG::Join(" + to_string(pidx) + ",";
  for (int i=0; i<cidx.size(); i++) msg += " "+to_string(cidx[i]);
  msg += ",";
  for (int i=0; i<coefs.size(); i++) msg += " "+to_string(coefs[i]);
  msg += ")";
  if (ECDAG_DEBUG_ENABLE) cout << msg << endl;
  // debug end

  // 0. deal with childs
  vector<ECNode*> targetChilds;
  for (int i=0; i<cidx.size(); i++) { 
    int curId = cidx[i];
    // 0.0 check whether child exists in our ecNodeMap
    unordered_map<int, ECNode*>::const_iterator findNode = _ecNodeMap.find(curId);
    ECNode* curNode;
    if (findNode == _ecNodeMap.end()) {
      // child does not exists, we need to create a new one
      curNode = new ECNode(curId);
      // insert into ecNodeMap
      _ecNodeMap.insert(make_pair(curId, curNode));
    } else {
      // child exists
      curNode = _ecNodeMap[curId];
    }
    // 0.1 add curNode into targetChilds
    targetChilds.push_back(curNode);
    // 0.2 check whether curNode is in headers
    vector<int>::iterator findpos = find(_ecHeaders.begin(), _ecHeaders.end(), curId);
    if (findpos != _ecHeaders.end()) {
      // delete from headers
      _ecHeaders.erase(findpos);
    }
    // 0.3 increase refNo for curNode
    curNode->incRefNumFor(curId);
  }

  // 1. deal with root
  ECNode* rNode;
  if (_ecNodeMap.find(pidx) == _ecNodeMap.end()) {
    // pidx does not exists, create new one and add to headers
    rNode = new ECNode(pidx);
    _ecNodeMap.insert(make_pair(pidx, rNode));
    _ecHeaders.push_back(pidx);
  } else {
    // pidx exists, clean the pidx node
    rNode = _ecNodeMap[pidx];
    rNode->cleanChilds();
  }
  rNode->setChilds(targetChilds);
  rNode->addCoefs(pidx, coefs);

  // 2. deal with cluster
  vector<int> childs(cidx);
  sort(childs.begin(), childs.end());
  int clusterIdx = findCluster(childs);
  Cluster* curCluster;
  if (clusterIdx == -1) {
    // cluster does not exists, create new cluster
    curCluster = new Cluster(childs, pidx);
    _clusterMap.push_back(curCluster);
  } else {
    curCluster = _clusterMap[clusterIdx];
    curCluster->addParent(pidx);
  }
}

int ECDAG::BindX(vector<int> idxs) {

}

void ECDAG::BindY(int pidx, int cidx) {

}

vector<int> ECDAG::toposort() {
  vector<int> toret;
  
  // We maintain 3 data-structures for topological sorting
  unordered_map<int, vector<int>> child2Parent; // given childId, get the parent list of this child
  unordered_map<int, vector<int>> inNum2List; // given incoming number, get the nodeId List
  unordered_map<int, int> id2InNum; // given nodeId, figure out current inNum

  for (auto item: _ecNodeMap) {
    int nodeId = item.first;
    ECNode* curNode = item.second;
    vector<ECNode*> childNodes = curNode->getChildren();
    int inNum = childNodes.size();
    id2InNum.insert(make_pair(nodeId, inNum));

    // maintain child->parent
    for (auto item: childNodes) {
      int childId = item->getNodeId();
      if (child2Parent.find(childId) == child2Parent.end()) {
        vector<int> list={nodeId};
        child2Parent.insert(make_pair(childId, list));
      } else {
        child2Parent[childId].push_back(nodeId);
      }
    }

    // maintain inNum2List
    if (inNum2List.find(inNum) == inNum2List.end()) {
      vector<int> list={nodeId};
      inNum2List.insert(make_pair(inNum, list));
    } else {
      inNum2List[inNum].push_back(nodeId);
    }
  }

//  for (auto item: child2Parent) {
//    cout << "child: " << item.first << ", parentList: ";
//    for (auto p: item.second) cout << p << " ";
//    cout << endl;
//  }
//
//  for (auto item: inNum2List) {
//    cout << "inNum: " << item.first << ", List:";
//    for (auto nid: item.second) cout << " " << nid;
//    cout << endl;
//  }

  // in each iteration we check nodes with inNum = 0
  while (true) {
    if (inNum2List.find(0) == inNum2List.end()) break;
    if (inNum2List[0].size() == 0) break;

    // take out the list with incoming number equals to 0
    vector<int> zerolist = inNum2List[0];
//    cout << "zerolist1: ";
//    for (auto xl: zerolist) cout << xl << " ";
//    cout << endl;
    // add items in zerolist to toret list
    for (auto id: zerolist) toret.push_back(id);
    inNum2List[0].clear();
    for (auto id: zerolist) {
      // do something to corresponding parent
      vector<int> idparentlist = child2Parent[id];
//      cout << "parentlist for " << id << ": ";
//      for (auto xl: idparentlist) cout << xl << " ";
//      cout << endl;
      for (auto p: idparentlist) {
        // figure out current inNum
        int curInNum = id2InNum[p];
        // figure out updated inNum
        int updatedInNum = curInNum-1;
        // update id2InNum for p
        id2InNum[p] = updatedInNum;
//	cout << "inNum for " << p << ": " << curInNum << " -> " << updatedInNum << endl;

        // remove p from curInNum list
        vector<int>::iterator ppos = find(inNum2List[curInNum].begin(), inNum2List[curInNum].end(), p);
        inNum2List[curInNum].erase(ppos);
        // add to updated list
        if (inNum2List.find(updatedInNum) == inNum2List.end()) {
          vector<int> updatedlist={p};
          inNum2List.insert(make_pair(updatedInNum, updatedlist));
        } else {
          inNum2List[updatedInNum].push_back(p);
        }
      }
    }
  }
  return toret;
}

ECNode* ECDAG::getNode(int cidx) {
  assert (_ecNodeMap.find(cidx) != _ecNodeMap.end());
  return _ecNodeMap[cidx];
}

vector<int> ECDAG::getHeaders() {
  return _ecHeaders;
}

vector<AGCommand*> ECDAG::parseForOEC(unordered_map<int, unsigned int> cid2ip,
                                      string stripename, 
                                      int n, int k, int w, int num,
                                      unordered_map<int, pair<string, unsigned int>> objlist) {
  vector<AGCommand*> toret;
  vector<int> sortedList = toposort();
  for (int i=0; i<sortedList.size(); i++) {
    int cidx = sortedList[i];
    ECNode* node = getNode(cidx);
    unsigned int ip = cid2ip[cidx];
    node->parseForOEC(ip);
    AGCommand* cmd = node->parseAGCommand(stripename, n, k, w, num, objlist, cid2ip);
    cmd->dump();
    toret.push_back(cmd);
//    node->dumpRawTask();
  }
  return toret;
}

vector<AGCommand*> ECDAG::persist(unordered_map<int, unsigned int> cid2ip, 
                                  string stripename,
                                  int n, int k, int w, int num,
                                  unordered_map<int, pair<string, unsigned int>> objlist) {
  vector<AGCommand*> toret;
  // sort headers
  sort(_ecHeaders.begin(), _ecHeaders.end());
  int numblks = _ecHeaders.size()/w;
  for (int i=0; i<numblks; i++) {
    int sid = _ecHeaders[i];
    string objname = objlist[sid].first;
    unsigned int ip = objlist[sid].second;
    vector<int> prevCids;
    vector<unsigned int> prevLocs;
    for (int j=0; j<w; j++) {
      int cid = sid*w+j;
      unsigned int cip = cid2ip[cid];
      prevCids.push_back(cid);
      prevLocs.push_back(cip);
    }
    AGCommand* cmd = new AGCommand(); 
    cmd->buildType5(5, ip, stripename, w, num, w, prevCids, prevLocs, objname);
    cmd->dump();
    toret.push_back(cmd);
  }
  return toret;
}

void ECDAG::dump() {
  for (auto id : _ecHeaders) {
    _ecNodeMap[id] ->dump(-1);
    cout << endl;
  }
  for (auto cluster: _clusterMap) {
    cluster->dump();
  }
}

void ECDAG::refineTasks(int n, int k, int w) {
//  // check for type0: no refine
//  vector<int> toposortres = toposort();
//  for (auto cidx : toposortres) {
//    ECNode* node = _ecNodeMap[cidx];
//    for (auto item: node->getRefMap()) {
//      if (item.second > 1) return;
//    }
//  }
//
//  // check for type1: aggregate load tasks for sub-packetization
//  if (w > 1) {
//    unordered_map<int, vector<int>> sid2cids;
//    for (auto cidx: toposortres) {
//      ECNode* node = _ecNodeMap[cidx];
//      unordered_map<int, ECTask*> tasks = node->getTasks();
//      if (tasks.find(0) != tasks.end()) {
//        int sid = cidx/w;
//        if (sid2cids.find(sid) == sid2cids.end()) {
//          vector<int> curcidlist = {cidx};
//          sid2cids.insert(make_pair(sid, curcidlist));
//        } else {
//          sid2cids[sid].push_back(cidx);
//        }
//      }
//    }
//    // now we aggregate all load tasks to one node and remove load tasks from others
//    for (auto item: sid2cids) {
//      int sid = item.first;
//      vector<int> cidlist = item.second;
//      sort(cidlist.begin(), cidlist.end());
//      if (cidlist.size() > 1) {
//        int cid = cidlist[0];
//        unordered_map<int, ECTask*> tasks = _ecNodeMap[cid]->getTasks();
//        ECTask* load = tasks[0];
//        for (int i=1; i<cidlist.size(); i++) {
//          int curcid = cidlist[i];
//          int ref = _ecNodeMap[curcid]->getRefNumFor(curcid);
//          load->addIdx(curcid) ;
//          load->addRef(curcid, ref);
//          _ecNodeMap[curcid]->clearTasks();
//        }
//      }
//    }
//  } else {
//    // aggregate load, fetch and compute
//    unordered_map<int, int> tomerge;
//    for (auto cidx: toposortres) {
//      ECNode* node = _ecNodeMap[cidx];
//      unordered_map<int, ECTask*> tasks = node->getTasks();
//      unsigned int ip = node->getIp();
//      if (tasks.find(2) == tasks.end()) continue;
//      vector<int> children = tasks[2]->getChildren();
//      // check whether children is load task
//      int childid;
//      bool found = false;
//      for (auto childidx: children) {
//        ECNode* childnode = _ecNodeMap[childidx];
//        unordered_map<int, ECTask*> childtasks = childnode->getTasks();
//        if (childtasks.find(0) == childtasks.end()) continue; 
//        else {
//          if (childnode->getIp() == ip) {
//            childid = childidx;
//            break;
//          }
//        }
//      }
//      if (found) {
//        // add load task for cidx
//        // clear load task for childid
//        unordered_map<int, ECTask*> tasks = _ecNodeMap[childid]->getTasks(); 
//        ECTask* load = tasks[0];
//        
//      }
//    }
//  }
}
