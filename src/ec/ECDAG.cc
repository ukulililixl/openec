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
  cluster->setOpt(1);
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

void ECDAG::reconstruct(int opt) {
  if (opt == 0) {
    // enable BindX automatically
    Opt0();
  } else if (opt == 1) {
    Opt1();
  }
}

void ECDAG::optimize(int opt, 
                     unordered_map<int, pair<string, unsigned int>> objlist,                            
                     unordered_map<unsigned int, string> ip2Rack,                            
                     int ecn,
                     int eck,
                     int ecw) {
  if (opt == 0) {
    // enable BindX automatically to reduce network traffic
    Opt0();
  } else if (opt == 1) {
    Opt1();
  } else if (opt == 2) {
    unordered_map<int, string> cid2Rack;
    for (auto item: _ecNodeMap) {
//      ECNode* curnode = item.second;
//      unsigned int curip = curnode->getIp();
//      string rack = ip2Rack[curip];
//      int cid = item.first;
//      cout << "cid: " << cid << ", ip: " << RedisUtil::ip2Str(curip) << endl;
//      cid2Rack.insert(make_pair(cid, rack));
    }
    Opt2(cid2Rack);
  }
}

void ECDAG::optimize2(int opt, 
                     unordered_map<int, unsigned int>& cid2ip,   
                     unordered_map<unsigned int, string> ip2Rack,                            
                     int ecn, int eck, int ecw,
                     unordered_map<int, unsigned int> sid2ip,
                     vector<unsigned int> allIps,
                     bool locality) {
  if (opt == 2) {
    unordered_map<int, string> cid2Rack;
    for (auto item: _ecNodeMap) {
      int cid = item.first;
      unsigned int curip = cid2ip[cid];
      string rack = ip2Rack[curip];
      cid2Rack.insert(make_pair(cid, rack));
    }
    Opt2(cid2Rack);

    // after we run Opt2, we need to recheck ip in cid2ip
    vector<int> toposeq = toposort();
    for (int i=0; i<toposeq.size(); i++) {
      int curcid = toposeq[i];
      ECNode* cnode = getNode(curcid);
      vector<unsigned int> candidates = cnode->candidateIps(sid2ip, cid2ip, allIps, ecn, eck, ecw, locality);
      srand((unsigned)time(0));
      int randomidx = randomidx % candidates.size();
      unsigned int ip = candidates[randomidx];
      cid2ip.insert(make_pair(curcid, ip));
    }
  }
}


void ECDAG::Opt0() {
  // check all the clusters and enforce Bind
  for (auto curCluster: _clusterMap) {
    if (curCluster->getOpt() == -1) {
      BindX(curCluster->getParents());
      curCluster->setOpt(0);
    }
  } 
}

void ECDAG::Opt1() {
  // add constraint for computation nodes;
  for (auto curCluster: _clusterMap) {
    if (curCluster->getOpt() == -1) {
      vector<int> childs = curCluster->getChilds();
      vector<int> parents = curCluster->getParents();
      srand((unsigned)time(0));
      int randomidx = rand() % childs.size();
      if (parents.size() == 1) {
        // addConstraint? 
        BindY(parents[0], childs[randomidx]);
      } else if (parents.size() > 1) {
        int bindnodeid = BindX(parents);
        BindY(bindnodeid, childs[randomidx]);
      }
      curCluster->setOpt(1);
    }
  }
}

void ECDAG::Opt2(unordered_map<int, string> n2Rack) {
  // 1. now we have all cids corresponding racks, iterate through all clusters
  vector<int> deletelist;
  for (int clusteridx = 0; clusteridx < _clusterMap.size(); clusteridx++) {
    Cluster* curCluster = _clusterMap[clusteridx];
    if (curCluster->getOpt() != -1) continue;
    if (ECDAG_DEBUG_ENABLE) cout << "ECDAG::Opt2.deal with ";
    if (ECDAG_DEBUG_ENABLE) curCluster->dump();
    vector<int> curChilds = curCluster->getChilds();
    vector<int> curParents = curCluster->getParents();
    int numoutput = curParents.size();

    // 1.1 sort the childs into racks
    unordered_map<string, vector<int>> subchilds;
    for (int i=0; i<curChilds.size(); i++) {
      string r = n2Rack[curChilds[i]];
      if (subchilds.find(r) == subchilds.end()) {
        vector<int> tmp={curChilds[i]};
        subchilds.insert(make_pair(r, tmp));
      } else subchilds[r].push_back(curChilds[i]);
    }
    if (ECDAG_DEBUG_ENABLE) cout << "ECDAG::childs are sorted into " << subchilds.size() << " racks" << endl;

    if (numoutput == 1) {
      // we deploy pipelining technique for current cluster
      if (ECDAG_DEBUG_ENABLE) cout << "numoutput == 1, deploy pipelining optimization" << endl;
      int parent = curParents[0];
      ECNode* parentnode = _ecNodeMap[parent];
      bool isProot = false;
      if (find(_ecHeaders.begin(), _ecHeaders.end(), parent) != _ecHeaders.end()) isProot = true;
      string prack = n2Rack[parent];

      // clean ref for child
      for (auto curcid: curChilds) {
        ECNode* curcnode = _ecNodeMap[curcid];
        curcnode->cleanRefNumFor(curcid);
      }

      deque<int> dataqueue;
      deque<int> coefqueue;

      // version 1
      for (auto group: subchilds) {
        string crack = group.first;
        if (crack == prack) continue;
        for (auto c: group.second) {
          dataqueue.push_back(c);
          int curcoef = parentnode->getCoefOfChildForParent(c, parent);
          coefqueue.push_back(curcoef);
        }
      }

      // for version 1 and version 2
      if (subchilds.find(prack) != subchilds.end()) {
        for (auto c: subchilds[prack]) {
          dataqueue.push_back(c);
          int curcoef = parentnode->getCoefOfChildForParent(c, parent);
          coefqueue.push_back(curcoef);
        }
      }
      while (dataqueue.size() >= 2) {
        vector<int> datav;
        vector<int> coefv;
        for (int j=0; j<2; j++) {
          int tmpd(dataqueue.front()); dataqueue.pop_front();
          int tmpc(coefqueue.front()); coefqueue.pop_front();
          datav.push_back(tmpd);
          coefv.push_back(tmpc);
        }
        int tmpid;
        if (dataqueue.size() > 0) tmpid = _optId++;
        else {
          tmpid = parent;
        }
        Join(tmpid, datav, coefv);
        BindY(tmpid, datav[1]);
        dataqueue.push_front(tmpid);
        coefqueue.push_front(1);
      }
      // add curCluster to delete list
      deletelist.push_back(clusteridx);
      
      if (!isProot) {
        // delete parent from root
        vector<int>::iterator p = find(_ecHeaders.begin(), _ecHeaders.end(), parent);
        if (p != _ecHeaders.end()) _ecHeaders.erase(p);
      }
    } else {
  
      if (subchilds.size() == 1) {
        BindX(curParents);
        continue;  // there is no space for current group to optimize
      }
  
      // 1.2 globalChilds and globalCoefs maintains the outer cluster.
      vector<int> globalChilds;
      unordered_map<int, vector<int>> globalCoefs;
  
      bool update = false;
      // 1.3 check for each group of subchilds for subcluster
      for (auto item: subchilds) {
        vector<int> itemchilds = item.second;
        vector<int> subparents;
        
        if (ECDAG_DEBUG_ENABLE) {
          cout << "deal with subgroup ( ";
          for (auto cid: itemchilds) cout << cid <<" ";
          cout << "), rack: " << item.first << endl;
        }
  
        if (itemchilds.size() > numoutput) {
          update = true;
          if (ECDAG_DEBUG_ENABLE) cout << "inputsize = " << itemchilds.size() << ", outputsize = " << numoutput << ", there is space for optimization" << endl;
          // we will reconstruct this group, clean ref for all itemchilds
          for (int i=0; i<itemchilds.size(); i++) {
            ECNode* itemchildnode = _ecNodeMap[itemchilds[i]];
            itemchildnode->cleanRefNumFor(itemchilds[i]);
          }
          // we can create a new subcluster for this group of childs, add subparents
          for (int i=0; i<numoutput; i++) {
            int tmpid = _optId++;
            subparents.push_back(tmpid);
            globalChilds.push_back(tmpid);        // update globalChilds here
          }
  
          if (ECDAG_DEBUG_ENABLE) {
            cout << "updated subparents: ( ";
            for (auto cid: subparents) cout << cid << " ";
            cout << ")" << endl;
            cout << "updated globalchilds: ( ";
            for (auto cid: globalChilds) cout << cid << " ";
            cout << ")" << endl;
          }
  
          // for each global parent, we need to figure out corresponding coefs to create tmpparent
          for (int i=0; i<numoutput; i++) {
            int parent = curParents[i];
            ECNode* parentnode = _ecNodeMap[parent];
            int tmpparent = subparents[i];
  
            vector<int> tmpcoef;
            // find corresponding coefs to calculate tmpparent 
            for (int i=0; i<itemchilds.size(); i++) {
              int tmpchild = itemchilds[i];
              int tmpc = parentnode->getCoefOfChildForParent(tmpchild, parent);
              tmpcoef.push_back(tmpc);
            }
            // for each itemchid, ref-=1
            Join(tmpparent, itemchilds, tmpcoef);
            if (ECDAG_DEBUG_ENABLE) {
              cout << tmpparent << " = ( ";
              for (auto c: tmpcoef) cout << c << " ";
              cout << ") * ( ";
              for (auto c: itemchilds) cout << c << " ";
              cout << ")" << endl;
              cout << "current cluster.size = " << _clusterMap.size() << endl;
            }

            // update globalCoefs here
            if (globalCoefs.find(parent) == globalCoefs.end()) {
              vector<int> tmp;
              globalCoefs.insert(make_pair(parent, tmp));
            }
            for (int j=0; j<numoutput; j++) {
              if (j == i) globalCoefs[parent].push_back(1);
              else globalCoefs[parent].push_back(0);
            }
          }
          int bindid=BindX(subparents);
          BindY(bindid, itemchilds[0]); // we also need to update in cid2ip
        } else {
          // we just pass itemchilds and corresponding coefs for parent
          for (int i=0; i<itemchilds.size(); i++) {
            globalChilds.push_back(itemchilds[i]);
            ECNode* itemchildnode = _ecNodeMap[itemchilds[i]];
            itemchildnode->cleanRefNumFor(itemchilds[i]);
          }
          for (int i=0; i<numoutput; i++) {
            int parent = curParents[i];
            ECNode* parentnode = _ecNodeMap[parent];
            // find corresponding coefs to calculate parent
            for (int j=0; j<itemchilds.size(); j++) {
              int tmpchild = itemchilds[j];
              int tmpc = parentnode->getCoefOfChildForParent(tmpchild, parent);
              if (globalCoefs.find(parent) == globalCoefs.end()) {
                vector<int> tmp;
                globalCoefs.insert(make_pair(parent, tmp));
              }
              globalCoefs[parent].push_back(tmpc);
            }
          }
        }
      } 
      // 1.4 update globalChilds and globalCoefs for current cluster
//      if (update) {
        for (int i=0; i<curParents.size(); i++) {
          int parent = curParents[i];
          assert (globalCoefs.find(parent) != globalCoefs.end());
          vector<int> coefs = globalCoefs[parent];
          Join(parent, globalChilds, coefs);
        }
        BindX(curParents);
        deletelist.push_back(clusteridx);
//      } else {
//        BindX(curParents);
//      }
  
      // check whether global Childs are in roots
      for (auto c: globalChilds) {
        vector<int>::iterator inroot = find(_ecHeaders.begin(), _ecHeaders.end(), c);
        if (inroot != _ecHeaders.end()) _ecHeaders.erase(inroot);
      }
    }
  }
  // delete cluster in deletelist
  vector<Cluster*>::iterator it;;
  sort(deletelist.begin(), deletelist.end());
  for (int i=deletelist.size()-1; i >= 0; i--) {
    it = _clusterMap.begin();
    int idx = deletelist[i];
    it += idx;
    _clusterMap.erase(it);
  }
}

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
    if (cmd) agCmds.insert(make_pair(cidx, cmd));
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
        if (agCmds.find(childCid) == agCmds.end()) continue;
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
      if (ECDAG_DEBUG_ENABLE) cout << "ECDAG::parseForOEC.merge " << cid << " and " << childid << endl;
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
          if (readCidListRef.find(tmpc) != readCidListRef.end()) {
            readCidListRef[tmpc]--;
            if (readCidListRef[tmpc] == 0) {
              readCidListRef.erase(tmpc);
            }
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
  if (ECDAG_DEBUG_ENABLE) cout << "ECDAG:: persist. numblks: " << numblks << endl;
  for (int i=0; i<numblks; i++) {
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
