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
  if (idxs.size() <= 1) return -1;
  // 0. create a bind node
  int bindid = _bindId++;
  assert (_ecNodeMap.find(bindid) == _ecNodeMap.end());
  ECNode* bindNode = new ECNode(bindid);
  // 1. we need to make sure for each node in idxs, their child are the same
  vector<int> childids;
  vector<ECNode*> childnodes; 
  assert (idxs.size() > 0);
  assert (_ecNodeMap.find(idxs[0]) != _ecNodeMap.end()); 
  childnodes = _ecNodeMap[idxs[0]]->getChildren();
  for (int i=0; i<childnodes.size(); i++) childids.push_back(childnodes[i]->getNodeId());
  bindNode->setChilds(childnodes);
  for (int i=1; i<idxs.size(); i++) {
    assert(_ecNodeMap.find(idxs[i]) != _ecNodeMap.end());
    ECNode* curnode = _ecNodeMap[idxs[i]];
    vector<ECNode*> curchildnodes = curnode->getChildren();
    assert (curchildnodes.size() == childids.size());
    for (int j=0; j<curchildnodes.size(); j++) {
      int curNodeID = curchildnodes[j]->getNodeId();
      assert (find(childids.begin(), childids.end(), curNodeID) != childids.end());
    }
  }
  // now we make sure that for each node in idxs, their child are the same
  // childids contains the nodeIds, and childnodes contains the child nodes
  // 2. for each node in idxs, figure out corresponding coef
  for (int i=0; i<idxs.size(); i++) {
    int tbid = idxs[i];
    assert (_ecNodeMap.find(tbid) != _ecNodeMap.end());
    ECNode* tbnode = _ecNodeMap[tbid];
    // add the coef of cur node to the bind node
    for (auto item: tbnode->getCoefmap()) {
      bindNode->addCoefs(item.first, item.second);
    }
    // add the ref of cur node to the bind node
    bindNode->setRefNum(tbid, tbnode->getRefNumFor(tbid));  // it seems no use here
    // clean the childs of cur node and add bind node as child
    tbnode->cleanChilds();
    vector<ECNode*> newChildNodes = {bindNode};
    tbnode->setChilds(newChildNodes);
    tbnode->addCoefs(tbid, {1});
  }
  // 3. add tbnode into ecmap
  _ecNodeMap.insert(make_pair(bindid, bindNode));
  // 4. deal with cluster
  int clusterid = findCluster(childids);
  assert (clusterid != -1);
  Cluster* cluster = _clusterMap[clusterid];
  // TODO: set optimization?
  cluster->setOpt(0); // BindX has opt level 0;

  // update ref for childnodes?
  for (int i=0; i<childnodes.size(); i++) {
    int curcid = childids[i];
    int curref = childnodes[i]->getRefNumFor(curcid);
    childnodes[i]->setRefNum(curcid, curref-idxs.size()+1);
  }
  return bindid;
}

void ECDAG::BindY(int pidx, int cidx) {
  unordered_map<int, ECNode*>::const_iterator curNode = _ecNodeMap.find(pidx);
  assert (curNode != _ecNodeMap.end());
  ECNode* toaddNode = _ecNodeMap[pidx];
  vector<ECNode*> childNodes = toaddNode->getChildren();

  vector<int> childids;
  for (int i=0; i<childNodes.size(); i++) childids.push_back(childNodes[i]->getNodeId());  
 
  curNode = _ecNodeMap.find(cidx);
  assert (curNode != _ecNodeMap.end());
  ECNode* consNode = _ecNodeMap[cidx];

  toaddNode->setConstraint(true, cidx);

  // deal with cluster
  int clusterid = findCluster(childids);
  assert (clusterid != -1);
  Cluster* cluster = _clusterMap[clusterid];
  // TODO: set optimization?
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

vector<int> ECDAG::getLeaves() {
  vector<int> toret;
  for (auto item: _ecNodeMap) {
    int idx = item.first;
    ECNode* node = item.second;
    if (node->getChildNum() == 0) toret.push_back(idx);
  }
  sort(toret.begin(), toret.end());
  return toret;
}

//vector<AGCommand*> ECDAG::parseForOEC(unordered_map<int, unsigned int> cid2ip,
unordered_map<int, AGCommand*> ECDAG::parseForOEC(unordered_map<int, unsigned int> cid2ip,
                                      string stripename, 
                                      int n, int k, int w, int num,
                                      unordered_map<int, pair<string, unsigned int>> objlist) {

  // adjust refnum for heads
  for (int i=0; i<_ecHeaders.size(); i++) {
    int nid = _ecHeaders[i];
    _ecNodeMap[nid]->incRefNumFor(nid);
  }

  vector<AGCommand*> toret;
  vector<int> sortedList = toposort();

  // we first put commands into a map
  unordered_map<int, AGCommand*> agCmds;
  for (int i=0; i<sortedList.size(); i++) {
    int cidx = sortedList[i];
    ECNode* node = getNode(cidx);
    unsigned int ip = cid2ip[cidx];
    node->parseForOEC(ip);
    AGCommand* cmd = node->parseAGCommand(stripename, n, k, w, num, objlist, cid2ip);
//    if (cmd) cmd->dump();
    if (cmd) agCmds.insert(make_pair(cidx, cmd));
//    toret.push_back(cmd);
  }

  // we can merge the following commands
  // 2. load & cache
  // 3. fetch & compute & cache
  // when ip of these two commands are the same
  if (w == 1) {
    unordered_map<int, int> tomerge;
    for (auto item: agCmds) {
      int cid = item.first;
      int sid = cid/w;
      AGCommand* cmd = agCmds[cid];
      if (cmd->getType() != 3) continue;
      // now we start with a type 3 command
      unsigned int ip = cmd->getSendIp(); 
      // check child
      int childid;
      AGCommand* childCmd;
      bool found = false;
      vector<int> prevCids = cmd->getPrevCids();
      vector<unsigned int> prevLocs = cmd->getPrevLocs();
      for (int i=0; i<prevCids.size(); i++) {
        int childCid = prevCids[i];
        unsigned int childip = prevLocs[i];
        AGCommand* cCmd = agCmds[childCid];
        if (cCmd->getType() != 2) continue;
        if (childip == ip) {
          childid = childCid;
          childCmd = cCmd;
          found = true;
          break;
        }
      }
      if (!found) continue;
      // we find a child that shares the same location with the parent
      tomerge.insert(make_pair(cid, childid));
    }
    // now in tomerge, item.first and item.second are sent to the same node, 
    // we can merge them into a single command to avoid local redis I/O
    for (auto item: tomerge) {
      int cid = item.first;
      int childid = item.second;
      cout << "ECDAG::parseForOEC.merge " << cid << " and " << childid << endl;
      AGCommand* cmd = agCmds[cid];
      AGCommand* childCmd = agCmds[childid];
      // get information from existing commands
      unsigned int ip = cmd->getSendIp();
      string readObjName = childCmd->getReadObjName();
      vector<int> readCidList = childCmd->getReadCidList();  // actually, when w=1, there is only one symbol in readCidList
      unordered_map<int, int> readCidListRef = childCmd->getCacheRefs();
//      assert (readCidList.size() == 1);
      int nprev = cmd->getNprevs();
      vector<int> prevCids = cmd->getPrevCids();
      vector<unsigned int> prevLocs = cmd->getPrevLocs();
      unordered_map<int, int> computeCidRef = cmd->getCacheRefs();
      unordered_map<int, vector<int>> computeCoefs = cmd->getCoefs();
      // update prevLocs
      vector<int> reduceList;
      for (int i=0; i<prevCids.size(); i++) {
        if (find(readCidList.begin(), readCidList.end(), prevCids[i]) != readCidList.end()) {
          reduceList.push_back(prevCids[i]);
          prevLocs[i] = 0; // this means we can get in the same process
        }
      }
      // update refs
      // considering that if ref in readCidList is larger than 1, we also need to cache it in redis
      for (auto item: computeCoefs) {
        for (auto tmpc: reduceList) {
          readCidListRef[tmpc]--;
          if (readCidListRef[tmpc] == 0) {
            readCidListRef.erase(tmpc);
          }
        }
      }
      // now we can merge childCmd and curCmd into a new command
      unordered_map<int, int> mergeref;
      for (auto item: readCidListRef) mergeref.insert(item);
      for (auto item: computeCidRef) mergeref.insert(item);
      AGCommand* mergeCmd = new AGCommand();
      mergeCmd->buildType7(7, ip, stripename, w, num, readObjName, readCidList, nprev, prevCids, prevLocs, computeCoefs, mergeref);
      // remove cid and childid commands in agCmds and add this command
      agCmds.erase(cid);
      agCmds.erase(childid);
      agCmds.insert(make_pair(cid, mergeCmd));
    }
  } 

  for (auto item: agCmds) item.second->dump();

  return agCmds;
}

vector<AGCommand*> ECDAG::persist(unordered_map<int, unsigned int> cid2ip, 
                                  string stripename,
                                  int n, int k, int w, int num,
                                  unordered_map<int, pair<string, unsigned int>> objlist) {
  vector<AGCommand*> toret;
  // sort headers
  sort(_ecHeaders.begin(), _ecHeaders.end());
  int numblks = _ecHeaders.size()/w;
  cout << "ECDAG:: persist. numblks: " << numblks << endl;
  for (int i=0; i<numblks; i++) {
//    int sid = _ecHeaders[i*w];
    int cid = _ecHeaders[i*w];
    int sid = cid/w;
    string objname = objlist[sid].first;
    unsigned int ip = objlist[sid].second;
    vector<int> prevCids;
    vector<unsigned int> prevLocs;
    for (int j=0; j<w; j++) {
      int cid = sid*w+j;
      ECNode* cnode = getNode(cid);
      unsigned int cip = cnode->getIp();
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
