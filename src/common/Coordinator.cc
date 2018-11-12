#include "Coordinator.hh"

Coordinator::Coordinator(Config* conf, StripeStore* ss) : _conf(conf) {
  // create local context
  try {
    _localCtx = RedisUtil::createContext(_conf -> _localIp);
  } catch (int e) {
    // TODO: error handling
    cerr << "initializing redis context to " << " error" << endl;
  }
  _stripeStore = ss;
//  _underfs = FSUtil::createFS(_conf->_fsType, _conf->_fsFactory[_conf->_fsType], _conf);
  srand((unsigned)time(0));
}

Coordinator::~Coordinator() {
  redisFree(_localCtx);
}

void Coordinator::doProcess() {
  redisReply* rReply;
  while (true) {
    cout << "Coordinator::doProcess" << endl;
    // will never stop looping
    rReply = (redisReply*)redisCommand(_localCtx, "blpop coor_request 0");
    if (rReply -> type == REDIS_REPLY_NIL) {
      cerr << "Coordinator::doProcess() get feed back empty queue " << endl;
    } else if (rReply -> type == REDIS_REPLY_ERROR) {
      cerr << "Coordinator::doProcess() get feed back ERROR happens " << endl;
    } else {
      cout << "Coordinator::doProcess() receive a request!" << endl;
      char* reqStr = rReply -> element[1] -> str;
      CoorCommand* coorCmd = new CoorCommand(reqStr);
      coorCmd->dump();
      int type = coorCmd->getType();
      switch (type) {
        case 0: registerFile(coorCmd); break;
        case 1: getLocation(coorCmd); break;
        case 2: finalizeFile(coorCmd); break;
        case 7: setECStatus(coorCmd); break;
//        case 3: getFileMeta(coorCmd); break;
//        case 4: offlineEnc(coorCmd); break;
//        case 5: offlineDegraded(coorCmd); break;
//        case 6: reportLost(coorCmd); break;
//        case 7: enableEncoding(coorCmd); break;
//        case 8: repairReqFromSS(coorCmd); break;
//        case 9: enableRepair(coorCmd); break;
//        case 10: onlineDegradedUpdate(coorCmd); break;
//        case 11: reportRepaired(coorCmd); break;
//        default: break;
      }

//      // delete coorCmd
//      delete coorCmd;
    }
    // free reply object
    freeReplyObject(rReply);
  }
}

void Coordinator::registerFile(CoorCommand* coorCmd) {
  unsigned int clientIp = coorCmd->getClientip();
  string filename = coorCmd->getFilename();
  string ecid = coorCmd->getEcid();
  int mode = coorCmd->getMode();
  int filesizeMB = coorCmd->getFilesizeMB();

  if (mode == 0) registerOnlineEC(clientIp, filename, ecid, filesizeMB);
  else if (mode == 1) registerOfflineEC(clientIp, filename, ecid, filesizeMB);
}

void Coordinator::registerOnlineEC(unsigned int clientIp, string filename, string ecid, int filesizeMB) {
  // 0. make sure that there is no existing ssentry
  assert (!_stripeStore->existEntry(filename));
  // 1. get ec instance 
  ECPolicy* ecpolicy = _conf->_ecPolicyMap[ecid];
  ECBase* ec = ecpolicy->createECClass();
  int ecn = ecpolicy->getN();
  int eck = ecpolicy->getK();
  int ecw = ecpolicy->getW();
  // 2. call ec Place method to get the group
  vector<vector<int>> group;
  ec->Place(group);
  // 3. sort group to a map, such that we can find corresponding group based on idx
  unordered_map<int, vector<int>> idx2group;
  for (auto item: group) {
    for (auto idx: item) {
      idx2group.insert(make_pair(idx, item));
    }
  }
  // 4. preassign location for each index
  vector<unsigned int> ips;
  vector<int> placed;
  vector<string> objnames;
  for (int i=0; i<ecn; i++) {
    string obj = filename+"_oecobj_"+to_string(i);
    objnames.push_back(obj);
    vector<int> colocWith;
    if (idx2group.find(i) != idx2group.end()) colocWith = idx2group[i];
    vector<unsigned int> candidates = getCandidates(ips, placed, colocWith);
    unsigned int curIp; // choose from candidates
    if (_conf->_avoid_local) {
      vector<unsigned int>::iterator position = find(candidates.begin(), candidates.end(), clientIp);
      if (position != candidates.end()) candidates.erase(position);
      curIp = chooseFromCandidates(candidates, _conf->_data_policy, "data");
    } else {
      if (find(candidates.begin(), candidates.end(), clientIp) != candidates.end()) curIp = clientIp;
      else curIp = chooseFromCandidates(candidates, _conf->_data_policy, "data");
    }
    placed.push_back(i);
    ips.push_back(curIp);
  }
  // 5. update ssentry and stripestore
  SSEntry* ssentry = new SSEntry(filename, 0, filesizeMB, ecid, objnames, ips);
  _stripeStore->insertEntry(ssentry);
  ssentry->dump();
  // 6. parse ECDAG and create commands for online encoding
  ECDAG* ecdag = ec->Encode();  
  vector<int> toposeq = ecdag->toposort();
  cout << "toposeq: ";
  for (int i=0; i<toposeq.size(); i++) cout << toposeq[i] << " ";
  cout << endl;

  // for online encoding, we assume k load tasks for original data
  // n persist tasks for encoded-obj 
  // we only need to tell OECAgent how to calculate parity
  vector<ECTask*> computetasks;
  for (int i=0; i<toposeq.size(); i++) {
    ECNode* curnode = ecdag->getNode(toposeq[i]);
    curnode->parseForClient(computetasks);
  }
  for (int i=0; i<computetasks.size(); i++) computetasks[i]->dump();
  
  // 9. send to agent instructions
  AGCommand* agCmd = new AGCommand();
  agCmd->buildType10(10, ecn, eck, ecw, computetasks.size());
  agCmd->setRkey("registerFile:"+filename);
  agCmd->sendTo(clientIp);
  delete agCmd;
  
  // 10. send compute tasks
  for (int i=0; i<computetasks.size(); i++) {
    ECTask* curcompute = computetasks[i];
    curcompute->buildType2();
    string key = "compute:"+filename+":"+to_string(i);
    curcompute->sendTo(key, clientIp);
  }
}

void Coordinator::registerOfflineEC(unsigned int clientIp, string filename, string ecpoolid, int filesizeMB) {
  // 0. make sure that there is no existing ssentry
  assert (!_stripeStore->existEntry(filename));
  // 1. given ecpoolid, figure out whether there is offline pool created in stripe store
  assert(_conf->_offlineECMap.find(ecpoolid) != _conf->_offlineECMap.end());
  assert(_conf->_offlineECBase.find(ecpoolid) != _conf->_offlineECBase.end());
  string ecid = _conf->_offlineECMap[ecpoolid];
  int basesizeMB = _conf->_offlineECBase[ecpoolid];
  assert(_conf->_ecPolicyMap.find(ecid) != _conf->_ecPolicyMap.end());
  ECPolicy* ecpolicy = _conf->_ecPolicyMap[ecid];
  OfflineECPool* ecpool = _stripeStore->getECPool(ecpoolid, ecpolicy, basesizeMB);
  ecpool->lock();
  // 2. get placement group 
  ECBase* ec = ecpolicy->createECClass();
  vector<vector<int>> group;
  ec->Place(group);
  unordered_map<int, vector<int>> idx2group;
  for (auto item: group) {
    for (auto idx: item) {
      idx2group.insert(make_pair(idx, item));
    }
  }
  // 3. check number of object that is going to be created for this file
  int objnum = filesizeMB/basesizeMB;
  // 4. for each object, add into a stripe an preassign location
  if (filesizeMB%basesizeMB) objnum += 1;
  // fileobjnames and fileobjlocs are used to create SSEntry for this file
  vector<string> fileobjnames;
  vector<unsigned int> fileobjlocs;
  for (int objidx=0; objidx<objnum; objidx++) {
    string objname = filename+"_oecobj_"+to_string(objidx);
    fileobjnames.push_back(objname);

    // 4.1 get a stripe for obj 
    //   we get a stripename from ecpool, however ecpool does not add objname into it at this time
    string stripename = ecpool->getStripeForObj(objname); 
    //   given the stripe name, we get existing objlist for this stripename
    vector<string> stripeobjlist = ecpool->getStripeObjList(stripename);
    // stripeips records location indexed by erasure coding index for each split in the stripe
    vector<unsigned int> stripeips;
    // stripeplaced records the objnames that have been stored in this stripe
    vector<int> stripeplaced;
    // we check obj in stripeobjlist one by one to fill stripeips and stripeplaced
    for (int i=0; i<stripeobjlist.size(); i++) {
      string curobjname = stripeobjlist[i];
      // given curobjname, find ssentry of the original file for this curobjname
      // if this curobjname is from previous stored file, there must be an SSEntry in stripestore
      // else ssentry is NULL
      SSEntry* ssentry = _stripeStore->getEntryFromObj(curobjname);
      // given curobjname, find location recorded in ssentry
      unsigned int curip;
      bool find = false;
      if (ssentry != NULL) {
        curip = ssentry->getLocOfObj(curobjname);
        find = true;
      } else {
        // curobjname is in current file
        for (int j=0; j<fileobjnames.size(); j++) {
          if (curobjname == fileobjnames[j]) {
            curip = fileobjlocs[j];
            find = true;
            break;
          }
        }
      }
      assert (find);
      // add this ip to stripeips
      stripeips.push_back(curip);
      // add stripeidx to stripeplaced
      stripeplaced.push_back(i);
    }

    // 4.2 given stripeips and stripeplaced, also group information from erasure code, preassign location for $objname
    int stripeidx = stripeplaced.size();
    vector<int> colocWith;
    if (idx2group.find(stripeidx) != idx2group.end()) colocWith = idx2group[stripeidx];
    vector<unsigned int> candidates = getCandidates(stripeips, stripeplaced, colocWith);
    unsigned int curIp; // choose from candidates
    if (_conf->_avoid_local) {
      vector<unsigned int>::iterator position = find(candidates.begin(), candidates.end(), clientIp);
      if (position != candidates.end()) candidates.erase(position);
      curIp = chooseFromCandidates(candidates, _conf->_data_policy, "data");
    } else {
      if (find(candidates.begin(), candidates.end(), clientIp) != candidates.end()) curIp = clientIp;
      else curIp = chooseFromCandidates(candidates, _conf->_data_policy, "data");
    }

    // 1.3 now we have preassigned a location for this objname, add to fileobjlocs
    fileobjlocs.push_back(curIp);

    // 1.4 add objname to ecpool
    ecpool->addObj(objname, stripename);
  }
  
  // 2. update ssentry
  SSEntry* ssentry = new SSEntry(filename, 1, filesizeMB, ecpoolid, fileobjnames, fileobjlocs);
  _stripeStore->insertEntry(ssentry);
  ssentry->dump();

  ecpool->unlock();

  // 3. send to agent instructions
  AGCommand* agCmd = new AGCommand();
  agCmd->buildType11(11, objnum, basesizeMB);
  agCmd->setRkey("registerFile:"+filename);
  agCmd->sendTo(clientIp);
  delete agCmd;
}

vector<unsigned int> Coordinator::getCandidates(vector<unsigned int> placedIp, vector<int> placedIdx, vector<int> colocWith) {
  vector<unsigned int> toret;
  // 0. check colocWith
  // candidate should be within the same rack
  for (int i=0; i<colocWith.size(); i++) {
    int curIdx = colocWith[i];
    // check whether this idx has been placed
    // if this idx has been placed
    if (placedIp.size() > curIdx) {
      // then figure out 
      unsigned int curIp = placedIp[curIdx];
      // find corresponding rack for this ip
      string rack = _conf->_ip2Rack[curIp];
      // for all the ip in this rack, if :
      // 1> this ip is not in toret
      // 2> this ip is not in placedId
      for (auto item:_conf->_rack2Ips[rack]) {
        if (find(toret.begin(), toret.end(), item) == toret.end() && 
            find(placedIp.begin(), placedIp.end(), item) == placedIp.end()) toret.push_back(item);
      }
    }
  }
  // if there is no constraints in colocWith, we add all ips into toret except for placedIp
  if (toret.size() == 0) {
    for (auto item:_conf->_agentsIPs) {
      if (find(placedIp.begin(), placedIp.end(), item) == placedIp.end()) toret.push_back(item);
    }
  }
  return toret; 
}

unsigned int Coordinator::chooseFromCandidates(vector<unsigned int> candidates, string policy, string type) {
  if (policy == "random") {
    int randomidx = rand() % candidates.size();
    return candidates[randomidx];
  }
  assert (candidates.size() > 0);
  if (type == "control") {
    int minload = _stripeStore->getControlLoad(candidates[0]);
    unsigned int minip = candidates[0];
    for (int i=1; i<candidates.size(); i++) {
      unsigned int ip = candidates[i];
      int load = _stripeStore->getControlLoad(ip);
      if (load < minload) {
        minload = load;
        minip = ip;
      }
    }
    _stripeStore->increaseControlLoadMap(minip, 1);
    return minip;
  } else if (type == "data") {
    int minload = _stripeStore->getDataLoad(candidates[0]);
    unsigned int minip = candidates[0];
    for (int i=1; i<candidates.size(); i++) {
      unsigned int ip = candidates[i];
      int load = _stripeStore->getDataLoad(ip);
      if (load < minload) {
        minload = load;
        minip = ip;
      }
    }
    _stripeStore->increaseDataLoadMap(minip, 1);
    return minip;
  } else if (type == "repair") {
    int minload = _stripeStore->getRepairLoad(candidates[0]);
    unsigned int minip = candidates[0];
    for (int i=1; i<candidates.size(); i++) {
      unsigned int ip = candidates[i];
      int load = _stripeStore->getRepairLoad(ip);
      if (load < minload) {
        minload = load;
        minip = ip;
      }
    }
    _stripeStore->increaseRepairLoadMap(minip, 1);
    return minip;
  } else if (type == "encode") {
    int minload = _stripeStore->getEncodeLoad(candidates[0]);
    unsigned int minip = candidates[0];
    for (int i=1; i<candidates.size(); i++) {
      unsigned int ip = candidates[i];
      int load = _stripeStore->getEncodeLoad(ip);
      if (load < minload) {
        minload = load;
        minip = ip;
      }
    }
    _stripeStore->increaseEncodeLoadMap(minip, 1);
    return minip;
  } else {
    int randomidx = rand() % candidates.size();
    return candidates[randomidx];
  }
}

void Coordinator::getLocation(CoorCommand* coorCmd) {
  unsigned int clientIp = coorCmd->getClientip();
  string objname = coorCmd->getFilename();
  int numOfReplicas = coorCmd->getNumOfReplicas();
  unsigned int* toret = (unsigned int*)calloc(numOfReplicas, sizeof(unsigned int));
  // 0. figure out the objtype
  if (objname.find("oecobj") != string::npos) {
    // placement request for an oecobj file, which is a part of online-encoded file
    // 1. figure out the original file name
    string oecobj("_oecobj_");
    size_t cpos = objname.find(oecobj);
    string filename = objname.substr(0, cpos);
    // 2. figure out the objidx
    size_t idxpos = cpos + oecobj.size();  
    string idxstr = objname.substr(idxpos, objname.size() - idxpos);
    int idx = atoi(idxstr.c_str());
    // 3. get the ssentry given the filename
    if (_stripeStore->existEntry(filename)) {
      SSEntry* ssentry = _stripeStore->getEntry(filename);
      assert (ssentry != NULL);
      vector<unsigned int> ips = ssentry->getObjloc();
      toret[0] = ips[idx];
    } else {
      cout << "Coordinator::getLocation.ssentry for " << filename << " does not exist!!!" << endl;
    }
  }

  cout << "Coordinator::getLocation.return: ";
  for (int i=0; i<numOfReplicas; i++) {
    cout << RedisUtil::ip2Str(toret[i]) << " ";
  }
  cout << endl;
  // last. return ip
  redisReply* rReply;
  redisContext* clientCtx = RedisUtil::createContext(_conf->_coorIp);
  string wkey = "loc:"+objname;
  rReply = (redisReply*)redisCommand(clientCtx, "rpush %s %b", wkey.c_str(), toret, numOfReplicas*sizeof(unsigned int));
  freeReplyObject(rReply);
  redisFree(clientCtx);
  if (toret) free(toret);
}

void Coordinator::finalizeFile(CoorCommand* coorCmd) {
  string filename = coorCmd->getFilename();
  // 0. given filename, get ssentry for this file
  SSEntry* ssentry = _stripeStore->getEntry(filename);
  assert(ssentry != NULL);
  int type = ssentry->getType();
  assert(type == 1); // offline encoding
  string ecpoolid = ssentry->getEcidpool();
  vector<string> objlist = ssentry->getObjlist();

  // 1. for each obj in objlist, check whether corresponding stripe can be a candidate for offline encoding
  OfflineECPool* ecpool = _stripeStore->getECPool(ecpoolid);
  ecpool->lock();
  for (int i=0; i<objlist.size(); i++) {
    string objname = objlist[i];
    // finalize objname
    ecpool->finalizeObj(objname);
    string stripename = ecpool->getStripeForObj(objname);
    if (ecpool->isCandidateForEC(stripename)) {
      cout << "Coordinator::finalizeFile. stripe " << stripename << "is candidate for ec " << endl;
      _stripeStore->addEncodeCandidate(ecpoolid, stripename);
    }
  }
  ecpool->unlock();
}

void Coordinator::setECStatus(CoorCommand* coorCmd) {
  int op = coorCmd->getOp();
  string ectype = coorCmd->getECType();
  _stripeStore->setECStatus(op, ectype);
}

//void Coordinator::getFileMeta(CoorCommand* coorCmd) {
//  cout << "Coordinator::getFileMeta.filename = " << coorCmd->_filename <<endl;
//  
//  string filename = coorCmd->_filename;
//  unsigned int clientip = coorCmd->_clientIp;
//
//  // 0. get ssentry
//  SSEntry* ssentry = _stripeStore->getEntry(filename);
//  assert (ssentry != NULL);
//
//  // 1. create filemeta
//  char* filemeta = (char*)calloc(1024,sizeof(char));
//  int metaoff = 0;
//
//  // 2. get redundancy, filesizeMB, ecid
//  int redundancy = ssentry->_redundancyType;
//
//  if (redundancy == 0) {
//    string ecid = ssentry->_ecid;
//    int filesizeMB = ssentry->_filesizeMB;
//    // 2.1 redundancy type
//    int tmpr = htonl(redundancy);
//    memcpy(filemeta + metaoff, (char*)&tmpr, 4); metaoff += 4;
//    // 2.2 filesize
//    int tmpsize = htonl(filesizeMB);
//    memcpy(filemeta + metaoff, (char*)&tmpsize, 4); metaoff += 4;
//    // 2.3 ecid
//    int lenecid = ecid.size();
//    lenecid = htonl(lenecid);
//    memcpy(filemeta + metaoff, (char*)&lenecid, 4); metaoff += 4;
//    memcpy(filemeta + metaoff, ecid.c_str(), ecid.size()); metaoff += ecid.size();
//  } else if (redundancy == 1) {
//    int filesizeMB = ssentry->_filesizeMB;
//    string poolname = ssentry->_poolname;
//    OfflineECPool* ecpool = _stripeStore->getECPool(poolname);
//    string stripename = ecpool->getStripeForObj(filename);
//    
//    // 2.1 redundancy type
//    int tmpr = htonl(redundancy);
//    memcpy(filemeta + metaoff, (char*)&tmpr, 4); metaoff += 4;
//    // 2.2 filesize
//    int tmpsize = htonl(filesizeMB);
//    memcpy(filemeta + metaoff, (char*)&tmpsize, 4); metaoff += 4;
//    // 2.3 poolname???
//    int lenpoolname = poolname.size();
//    int tmplenpoolname = htonl(lenpoolname);
//    memcpy(filemeta + metaoff, (char*)&tmplenpoolname, 4); metaoff += 4;
//    memcpy(filemeta + metaoff, poolname.c_str(), poolname.size()); metaoff += poolname.size();
//    // 2.4 stripename???
//    int lenstripename = stripename.size();
//    int tmplenstripename = htonl(lenstripename);
//    memcpy(filemeta + metaoff, (char*)&tmplenstripename, 4); metaoff += 4;
//    memcpy(filemeta + metaoff, stripename.c_str(), stripename.size()); metaoff += stripename.size();
//  }
//
//  // 3. send to clientip
//  string key = "filemeta:"+filename;
//  redisContext* sendCtx = RedisUtil::createContext(clientip);
//  redisReply* rReply = (redisReply*)redisCommand(sendCtx, "RPUSH %s %b", key.c_str(), filemeta, metaoff);
//  freeReplyObject(rReply);
//  redisFree(sendCtx);
//
//  // free
//  if (filemeta) free(filemeta);
//}
//
//void Coordinator::offlineEnc(CoorCommand* coorCmd) {
//  cout << "Coordinator::offlineEnc" << endl;
//  string poolname = coorCmd->_poolname;
//  string stripename = coorCmd->_stripename;
//
//  // 0. given the poolname, get the ecpool
//  OfflineECPool* ecpool = _stripeStore->getECPool(poolname);
//  ECPolicy* ecpolicy = ecpool->_ecpolicy;
//  ECBase* ec = ecpolicy->createECClass();
//  int k=ec->_k;
//  int n=ec->_n;
//  int scratio=ec->_cps;
//  cout << "Coordinator::offlineEnc.poolname:" << poolname <<", stripename:" << stripename << ", k:" << k << ", n:" << n << ", scratio:" << scratio << endl;
//
//  // 1. prepare data and code vector
//  vector<int> data;
//  vector<int> code;
//  for (int i=0; i<k*scratio; i++) data.push_back(i);
//  for (int i=k*scratio; i<n*scratio; i++) code.push_back(i);
//
//  // 2. create encode ecdag
//  ec->encode(data, code);
//  ec->dump();
//
//  // 3. prepare objlist
//  unordered_map<int, pair<string, unsigned int>> objlist;
//  vector<string> stripelist = ecpool->getStripeObjList(stripename);
//  vector<int> placed;
//  vector<unsigned int> ips;
//  int filesizeMB=0;
//  for (int i=0; i<stripelist.size(); i++) {
//    int sid = i;
//    string objname = stripelist[i];
//    SSEntry* ssentry = _stripeStore->getEntry(objname);
//    unsigned int loc = ssentry->_objLocs[0];
//    pair<string, unsigned int> curpair = make_pair(objname, loc);
//    objlist.insert(make_pair(sid, curpair));
//    if (ssentry->_filesizeMB>filesizeMB) filesizeMB = ssentry->_filesizeMB;
//    ips.push_back(loc);
//    placed.push_back(sid);
//  }
//
//  cout << "Coordinator::offlineEnc.objlist (sid, objname, loc) = ";
//  for (auto item: objlist) {
//    int sid = item.first;
//    string objname = item.second.first;
//    unsigned int ip = item.second.second;
//    cout << "(" << sid << ", " << objname << ", " << RedisUtil::ip2Str(ip) << ") ";
//  }
//  cout << endl;
//
//  // 3.1 get the final bind node and preassign location for it.
//  int bindCid = ec->getBindCid();
//  if ((bindCid != -1) && (_conf->_encode_policy == "balance")) {
//    // we do some encoding job scheduling here
//    vector<unsigned int> candidates = _conf->_agentsIPs;
//    unsigned int curloc = chooseFromCandidates(candidates, _conf->_encode_policy, "encode");
//    pair<string, unsigned int> curpair = make_pair(to_string(bindCid), curloc);
//    objlist.insert(make_pair(bindCid/scratio, curpair));
//    cout << "Coordinator::offlineEnc.encode for " << stripename << " is assigned at " << RedisUtil::ip2Str(curloc) << endl;
//  }
//  
//  // 4. prepare num of slices
//  int num = filesizeMB * 1048576 / _conf->_pktSize;
//
//  // 5. denforce
//  unordered_map<int, bool> denforceMap;
//  unordered_map<int, AGCommand*> agCmds;
//  vector<unsigned int> allnodes = _conf->_agentsIPs;
//
//  // xiaolu add 20180828 start
//  ec->Optimize(ec->_opt, objlist, _conf->_ip2Rack, n, k, scratio);
//  // xiaolu add 20180828 end
//
//  ec->DEnforce(denforceMap, agCmds, objlist, stripename, n, k, scratio, num, allnodes, ec->_locality);
//
//  // xiaolu add 20180829 start
//  vector<vector<int>> group;
//  ec->place(group);
//  unordered_map<int, vector<int>> idx2group;
//  for (auto item: group) {
//    for (auto idx: item) {
//      idx2group.insert(make_pair(idx, item));
//    }
//  }
//  // xiaolu add 20180829 end
//  
//  // 6. create parityobj names and locs
//  unordered_map<int, pair<string, unsigned int>> paritylist;
//  for (int i=k; i<n; i++) {
//    string parityname = "/"+poolname+"-"+stripename+"-"+to_string(i);
//
//    vector<int> colocWith;
//    vector<int> notColocWith;
//
//    // xiaolu add 20180829 start
//    if (idx2group.find(i) != idx2group.end()) colocWith = idx2group[i];
//    if (colocWith.size() > 0) {
//      for (int j=0; j<ec->_n; j++) {
//        if (find(colocWith.begin(), colocWith.end(), j) == colocWith.end()) notColocWith.push_back(j);
//      }
//    }
//    // xiaolu add 20180829 end
//
//
//    // xiaolu comment 20180829 start
////    ec->placementRestriction(i, placed, colocWith, notColocWith);
//    // xiaolu comment 20180829 end
//
//    vector<unsigned int> candidates = getCandidates(ips, colocWith, notColocWith);
//    unsigned int curIp = chooseFromCandidates(candidates, _conf->_data_policy, "data");
//
//    placed.push_back(i);
//    ips.push_back(curIp);
//    pair<string, unsigned int> curpair = make_pair(parityname, curIp);
//    paritylist.insert(make_pair(i, curpair));
//
//    // now we have preassigned a location for this obj
//    // 6.1 add cur obj to ecpool
//    ecpool->addObj(parityname, stripename);
//    // 6.2 create ssentry
//    vector<unsigned int> assignedips;
//    assignedips.push_back(curIp);
//    SSEntry* ssentry = new SSEntry(parityname, 1, poolname, assignedips);
//    _stripeStore->insertEntry(ssentry);
//  }
//  cout << "Coordinator::offlineEnc.paritylist (sid, objname, loc) = ";
//  for (auto item: paritylist) {
//    int sid = item.first;
//    string objname = item.second.first;
//    unsigned int ip = item.second.second;
//    cout << "(" << sid << ", " << objname << ", " << RedisUtil::ip2Str(ip) << ") ";
//  }
//  cout << endl;
//
//  // 7. persists header
//  vector<AGCommand*> persistCmds;
//  ec->PersistHeader(agCmds, paritylist, stripename, n, k, scratio, num, persistCmds);
//
//
//  // xiaolu comment 20180825 start
//  // 8. send compute commands to distributor
//  vector<char*> todelete;
//  redisContext* distCtx = RedisUtil::createContext(_conf->_coorIp);
//
//  redisAppendCommand(distCtx, "MULTI");
//  for (auto item: agCmds) {
//    int cid = item.first;
//    AGCommand* agcmd = item.second;
//    unsigned int ip = agcmd->_sendIp;
//    ip = htonl(ip);
//    if (agcmd->_shouldSend) {
//      char* cmdstr = agcmd->_agCmd;
//      int cmLen = agcmd->_cmLen;
//      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
//      memcpy(todist, (char*)&ip, 4);
//      memcpy(todist+4, cmdstr, cmLen); 
//      todelete.push_back(todist);
//      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
//    }
//  }
//  // xiaolu commdn 20180825 end
//
////  // xiaolu add 20180825 start
////  vector<AGCommand*> reverseCmds;
////  for (auto item: agCmds) reverseCmds.push_back(item.second);
////
////  vector<char*> todelete;
////  redisContext* distCtx = RedisUtil::createContext(_conf->_coorIp);
////
////  redisAppendCommand(distCtx, "MULTI");
////  for (int i=reverseCmds.size()-1; i>=0; i--) {
////    AGCommand* agcmd = reverseCmds[i];
////    unsigned int ip = agcmd->_sendIp;
////    ip = htonl(ip);
////    if (agcmd->_shouldSend) {
////      char* cmdstr = agcmd->_agCmd;
////      int cmLen = agcmd->_cmLen;
////      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
////      memcpy(todist, (char*)&ip, 4);
////      memcpy(todist+4, cmdstr, cmLen); 
////      todelete.push_back(todist);
////      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
////    }
////  }
////  
////  // xiaolu add 20180825 end
//
//  for (auto agcmd: persistCmds) {
//    unsigned int ip = agcmd->_sendIp;
//    ip = htonl(ip);
//    if (agcmd->_shouldSend) {
//      char* cmdstr = agcmd->_agCmd;
//      int cmLen = agcmd->_cmLen;
//      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
//      memcpy(todist, (char*)&ip, 4);
//      memcpy(todist+4, cmdstr, cmLen); 
//      todelete.push_back(todist);
//      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
//    }
//  }
//  redisAppendCommand(distCtx, "EXEC");
//
//  redisReply* distReply;
//  redisGetReply(distCtx, (void **)&distReply);
//  freeReplyObject(distReply);
//  for (auto item: todelete) {
//    redisGetReply(distCtx, (void **)&distReply);
//    freeReplyObject(distReply);
//  }
//  redisGetReply(distCtx, (void **)&distReply);
//  freeReplyObject(distReply);
//  redisFree(distCtx);
//
//  // wait for finish flag?
//  for (auto agcmd: persistCmds) {
//    unsigned int ip = agcmd->_sendIp; 
//    redisContext* waitCtx = RedisUtil::createContext(ip);
//    string wkey = "writefinish:"+agcmd->_writeObjName;
//    redisReply* fReply = (redisReply*)redisCommand(waitCtx, "blpop %s 0", wkey.c_str());
//    freeReplyObject(fReply);
//    redisFree(waitCtx);
//  }
//  cout << "Coordinator::offlineEnc for " << stripename << " finishes" << endl;
//  _stripeStore->finishECStripe(stripename);
//
//  // last. delete
//  delete ec;
//  for (auto item: agCmds) delete item.second;
//  for (auto item: persistCmds) delete item;
//  for (auto item: todelete) free(item);
//}
//
//void Coordinator::onlineDegradedUpdate(CoorCommand* coorCmd) {
//  unsigned int clientIp = coorCmd->_clientIp;
//  string filename = coorCmd->_filename;
//  cout << "Coordinator::onlineDegradedUpdate for " << filename << endl;
//  
//  // update lostmap in stripe store
//  _stripeStore->addToLostMap(filename);
//}
//
//void Coordinator::offlineDegraded(CoorCommand* coorCmd) {
//  unsigned int clientIp = coorCmd->_clientIp;
//  string filename = coorCmd->_filename;
//  string poolname = coorCmd->_poolname;
//  string stripename = coorCmd->_stripename;
//  cout << "Coordinator::offlineDegraded.filename: " << filename << ", ip: " << RedisUtil::ip2Str(clientIp) << ", poolname: " << poolname << ", stripename: " << stripename << endl;
//
//  // update lostmap in stripe store
//  _stripeStore->addToLostMap(filename);
//
//  // 0. prepare
//  OfflineECPool* ecpool = _stripeStore->getECPool(poolname);
//  vector<string> stripeobjs = ecpool->getStripeObjList(stripename);
//  cout << "Coordinator::offlineDegraded.stripeobjs: ";
//  for (int i=0; i<stripeobjs.size(); i++) {
//    cout << stripeobjs[i] << " ";
//  }
//  cout << endl;
//
//  // 1. figure out the corrupted sidx and integrity
//  int sid;
//  vector<int> integrity;
//  unordered_map<int, pair<string, unsigned int>> objlist;
//  int filesizeMB = 0;
//  for (int i=0; i<stripeobjs.size(); i++) {
//    string curobj = stripeobjs[i];
//    if (filename.compare(curobj) == 0) {
//      sid = i;
//      integrity.push_back(0);
//      // as the client is reading the lost data at clientIp, so we set it to clientIp
//      pair<string, unsigned int> curpair = make_pair(curobj, clientIp);
//      objlist.insert(make_pair(i, curpair));
//    } else {
//      FSObjInputStream* curstream = new FSObjInputStream(_conf, curobj, _underfs);
//      if (curstream->exist()) integrity.push_back(1);
//      else integrity.push_back(0);
//      if  (curstream) delete curstream;
//      SSEntry* ssentry = _stripeStore->getEntry(curobj);
//      unsigned int loc = ssentry->_objLocs[0];
//      pair<string, unsigned int> curpair = make_pair(curobj, loc);
//      objlist.insert(make_pair(i, curpair));
//      if (ssentry->_filesizeMB > filesizeMB) filesizeMB = ssentry->_filesizeMB;
//    }
//  }
//
//  // 2. figure out avail and torec list
//  ECPolicy* ecpolicy = ecpool->_ecpolicy;
//  ECBase* ec = ecpolicy->createECClass();
//  int k = ec->_k;
//  int n = ec->_n;
//  int scratio = ec->_cps;
//
//  vector<int> torec;
//  vector<int> avail;
//  for (int i=0; i<scratio; i++) torec.push_back(sid*scratio+i);
//  for (int i=0; i<n; i++) {
//    if (integrity[i]) {
//      for (int j=0; j<scratio; j++) avail.push_back(i*scratio+j);
//    }
//  }
//  cout << "Coordinator::offlineDegraded.avail = ";
//  for (auto item: avail) cout << item << " ";
//  cout << endl;
//  cout << "Coordinator::offlineDegraded.torec = ";
//  for (auto item: torec) cout << item << " ";
//  cout << endl;
//
//  // 3. prepare decode ECDAG
//  ec->decode(avail, torec);
//  ec->dump();
//
//  // 4. DEnforce
//  int num = filesizeMB * 1048576 / _conf->_pktSize;
//  unordered_map<int, bool> denforceMap;
//  unordered_map<int, AGCommand*> agCmds;
//  vector<unsigned int> allnodes = _conf->_agentsIPs;
//
//  // xiaolu add 20180828 start
//  ec->Optimize(ec->_opt, objlist, _conf->_ip2Rack, n, k, scratio);
//  // xiaolu add 20180828 end
//
//  ec->DEnforce(denforceMap, agCmds, objlist, stripename, n, k, scratio, num, allnodes, ec->_locality);
//
//  // 5. tell client where to fetch
//  unordered_map<int, unsigned int> headerinfo;
//  ec->fetchHeader(headerinfo, agCmds);
//  
//  char* clistr = (char*)calloc(1024, sizeof(char));
//  int clioff = 0;
//  // 5.1 corrupted idx
//  int tmpsid = htonl(sid);
//  memcpy(clistr + clioff, (char*)&tmpsid, 4); clioff += 4;
//  // 5.2 scratio
//  int tmpscratio = htonl(scratio);
//  memcpy(clistr + clioff, (char*)&tmpscratio, 4); clioff += 4;
//  // 5.3 num of pkt
//  int tmpnum = htonl(num);
//  memcpy(clistr + clioff, (char*)&tmpnum, 4); clioff += 4;
//  // 5.4 location list
//  for (int i=0; i<scratio; i++) {
//    int cid = sid*scratio + i;
//    unsigned int ip = headerinfo[cid];
//    ip = htonl(ip);
//    memcpy(clistr + clioff, (char*)&ip, 4); clioff += 4;
//  }
//  redisReply* rReply;
//  redisContext* cliCtx = RedisUtil::createContext(clientIp);
//  string skey = "degraded:"+filename;
//  rReply = (redisReply*)redisCommand(cliCtx, "rpush %s %b", skey.c_str(), clistr, clioff);
//  freeReplyObject(rReply);
//  redisFree(cliCtx);
//  
//  if (clistr) free(clistr);
//
//  // 6. distribute cmds
//  vector<char*> todelete;
//  redisContext* distCtx = RedisUtil::createContext(_conf->_coorIp);
//
//  redisAppendCommand(distCtx, "MULTI");
//  for (auto item: agCmds) {
//    int cid = item.first;
//    AGCommand* agcmd = item.second;
//    unsigned int ip = agcmd->_sendIp;
//    ip = htonl(ip);
//    if (agcmd->_shouldSend) {
//      char* cmdstr = agcmd->_agCmd;
//      int cmLen = agcmd->_cmLen;
//      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
//      memcpy(todist, (char*)&ip, 4);
//      memcpy(todist+4, cmdstr, cmLen); 
//      todelete.push_back(todist);
//      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
//    }
//  }
//  redisAppendCommand(distCtx, "EXEC");
//
//  redisReply* distReply;
//  redisGetReply(distCtx, (void **)&distReply);
//  freeReplyObject(distReply);
//  for (auto item: todelete) {
//    redisGetReply(distCtx, (void **)&distReply);
//    freeReplyObject(distReply);
//  }
//  redisGetReply(distCtx, (void **)&distReply);
//  freeReplyObject(distReply);
//  redisFree(distCtx);
//
//  // last. delete
//  if (ec) delete ec;
//  for (auto item: agCmds) delete item.second;
//  for (auto item: todelete) free(item);
//}
//
//void Coordinator::reportRepaired(CoorCommand* coorCmd) {
//  string objname = coorCmd->_filename;
//  cout << "Coordinator::reportRepaired for " << objname << endl;
//  _stripeStore->finishRepair(objname);
//}
//
//void Coordinator::reportLost(CoorCommand* coorCmd) {
//  string objname = coorCmd->_filename;
//  cout << "Coordinator::reportLost for "   << objname << endl;
//
//  // xiaolu add 20180820 start
//  _stripeStore->addToLostMap(objname);
//  // xiaolu add 20180820 end
//
//  // xiaolu Comment 20180820 start
////  // 0. first check whether enable repair
////  //    if not enable repair, we add the lost obj to stripestore
////  if (_conf->_repair_scheduling == "delay") {
////    // add obj to stripestore
////    _stripeStore->addToLostMap(objname);
////    cout << "Coordinator::add " << objname << " to StripeStore" << endl;
////  } else if (_conf->_repair_scheduling == "threshold") {
////    _stripeStore->addToLostMap(objname);
////    cout << "Coordinator::add " << objname << " to StripeStore" << endl;
////  } else {
////    // repair on receiving the lost report
////    if (objname.find("oecobj") != string::npos) {
////      return recoveryOnline(objname);
////    } else {
////      return recoveryOffline(objname);
////    }
////  }
//  // xiaolu Comment 20180820 end
//}
//
//void Coordinator::repairReqFromSS(CoorCommand* coorCmd) {
//  string objname = coorCmd->_filename;
//  cout << "Coordinator::repairReqFromSS.repair request for " << objname << endl;
//
//  if (objname.find("oecobj") != string::npos) {
//    return recoveryOnline(objname);
//  } else {
//    return recoveryOffline(objname);
//  }
//}
//
//void Coordinator::recoveryOnline(string objname) {
//  cout << "Coordinator::recoveryOnline.repair " << objname << endl;
//  // 1. figure out the original file name
//  string oecobj("_oecobj_");
//  size_t cpos = objname.find(oecobj);
//  string filename = objname.substr(0, cpos);
//  // 2. figure out the objidx
//  size_t idxpos = cpos + oecobj.size();  
//  string idxstr = objname.substr(idxpos, objname.size() - idxpos);
//  int sid = atoi(idxstr.c_str());
//  cout << "Coordinator::recoveryOnline.filename: " << filename << ", sid: " << sid << endl;
//  // 3. given filename, prepare
//  SSEntry* ssentry = _stripeStore->getEntry(filename);
//  string ecid = ssentry->_ecid;
//  ECPolicy* ecpolicy = _conf->_ecPolicyMap[ecid];
//  ECBase* ec = ecpolicy->createECClass();
//
//  int k=ec->_k;
//  int n=ec->_n;
//  int scratio=ec->_cps;
//
//  vector<string> stripeobjs;
//  for (int i=0; i<n; i++) {
//    string curobj = filename + "_oecobj_" + to_string(i);
//    stripeobjs.push_back(curobj);
//    cout << "Coordinator::recoveryOnline.curobj: " << curobj << endl;
//  }
//
//  // 4. figure out integrity
//  vector<int> integrity;
//  unordered_map<int, pair<string, unsigned int>> objlist;
//  int filesizeMB = 0;
//  vector<int> relocateIdx;  // we need to relocate the location for these index.
//  for (int i=0; i<stripeobjs.size(); i++) {
//    string curobj = stripeobjs[i];
//    FSObjInputStream* curstream = new FSObjInputStream(_conf, curobj, _underfs);
//    // TODO: update location for repaired obj?
//    unsigned int loc;
//    if (curstream->exist()) {
//      integrity.push_back(1);
//      loc = ssentry->_objLocs[i];
//    } else {
//      integrity.push_back(0);
//      loc = ssentry->_objLocs[i];
//      relocateIdx.push_back(i);
//      // TODO: update the load balance statistics in stripestore?
//    }
//    pair<string, unsigned int> curpair = make_pair(curobj, loc);
//    objlist.insert(make_pair(i, curpair));
//    if (ssentry->_filesizeMB > filesizeMB) filesizeMB = ssentry->_filesizeMB;
//
//    if (curstream) delete curstream;
//  }
//  int objsizeMB = filesizeMB / k;
//  // set corruptidx
//  integrity[sid] = 0;
//
//  // debug
//  cout << "Coordinator::recoveryOnline.objlist: ";
//  for (int i=0; i<stripeobjs.size(); i++) {
//    cout << "[" << i << ", " << objlist[i].first << ", " << RedisUtil::ip2Str(objlist[i].second) << "] ";
//  }
//  cout << endl;
//  cout << "Coordinator::recoveryOnline.relocateIdx: ";
//  for (int i=0; i<relocateIdx.size(); i++) cout << relocateIdx[i] << " ";
//  cout << endl;
//
//  // xiaolu add 20180829 start
//  vector<vector<int>> group;
//  ec->place(group);
//  unordered_map<int, vector<int>> idx2group;
//  for (auto item: group) {
//    for (auto idx: item) {
//      idx2group.insert(make_pair(idx, item));
//    }
//  }
//  // xiaolu add 20180829 end
//
//  // relocate
//  sort(relocateIdx.begin(), relocateIdx.end());
//  int tmpi=0;
//  int curRelocateIdx=relocateIdx[tmpi];
//  vector<int> placed;
//  vector<unsigned int> ips;
//  for (int i=0; i<stripeobjs.size(); i++) {
//    if (i < curRelocateIdx) {
//      placed.push_back(i);
//      ips.push_back(objlist[i].second);
//    } else if (i == curRelocateIdx) {
//      vector<int> colocWith;
//      vector<int> notColocWith;
//
//      // xiaolu add 20180829 start
//      if (idx2group.find(i) != idx2group.end()) colocWith = idx2group[i];
//      if (colocWith.size() > 0) {
//        for (int j=0; j<ec->_n; j++) {
//          if (find(colocWith.begin(), colocWith.end(), j) == colocWith.end()) notColocWith.push_back(j);
//        }
//      }
//      // xiaolu add 20180829 end
//
//      // xiaolu comment 20180829 start
////      ec->placementRestriction(i, placed, colocWith, notColocWith);
//      // xiaolu comment 20180829 end
//
//      vector<unsigned int> candidates = getCandidates(ips, colocWith, notColocWith);
//      // we also need to remove the remaining nodes
//      for (int j=i+1; j<stripeobjs.size(); j++) {
//        if (find(relocateIdx.begin(), relocateIdx.end(), j) != relocateIdx.end()) continue;
//        else {
//          unsigned int toremove = objlist[j].second;
//          vector<unsigned int>::iterator position = find(candidates.begin(), candidates.end(), toremove);
//          if (position != candidates.end()) candidates.erase(position);
//        }
//      }
//      // now we choose a location from candidates
//      cout << "Coordinator::recoveryOnline.relocate for " << i << ", choose from candidates: ";
//      for (auto item: candidates) cout << RedisUtil::ip2Str(item) << " ";
//      cout << endl;
////      unsigned int curloc = chooseFromCandidates(candidates, _conf->_data_policy, "data");
//      unsigned int curloc = chooseFromCandidates(candidates, _conf->_repair_policy, "repair");
//      cout << "Coordinator::recoveryOnline.relocate " << i << " with " << RedisUtil::ip2Str(curloc) << endl;
//      placed.push_back(i);
//      ips.push_back(curloc);
//      // set curloc in objlist
//      objlist[i].second = curloc;
//      // update SSEntry
//      ssentry->setOnlineLoc(i, curloc);
//      // update curRelocateIdx
//      tmpi++;
//      if (tmpi >= relocateIdx.size()) break;
//      else curRelocateIdx = relocateIdx[tmpi];
//    }
//  }
//
//  cout << "Coordinator::recoveryOnline.updated objlist: ";
//  for (int i=0; i<stripeobjs.size(); i++) {
//    cout << "[" << i << ", " << objlist[i].first << ", " << objlist[i].second << "] ";
//  }
//  cout << endl;
//
//  // 4. prepare avail and torec
//  vector<int> torec;
//  vector<int> avail;
//  for (int i=0; i<scratio; i++) torec.push_back(sid*scratio+i);
//  for (int i=0; i<n; i++) {
//    if (integrity[i]) {
//      for (int j=0; j<scratio; j++) avail.push_back(i*scratio+j);
//    }
//  }
//  cout << "Coordinator::offlineDegraded.avail = ";
//  for (auto item: avail) cout << item << " ";
//  cout << endl;
//  cout << "Coordinator::offlineDegraded.torec = ";
//  for (auto item: torec) cout << item << " ";
//  cout << endl;
//
//  // 5. prepare decode ECDAG
//  ec->decode(avail, torec);
//  ec->dump();
//
//  // 6. DEnforce
//  string stripename = filename;
//  int num = objsizeMB * 1048576 / _conf->_pktSize;
//  unordered_map<int, bool> denforceMap;
//  unordered_map<int, AGCommand*> agCmds;
//  vector<unsigned int> allnodes = _conf->_agentsIPs;
//
//  // xiaolu add 20180828 start
//  ec->Optimize(ec->_opt, objlist, _conf->_ip2Rack, n, k, scratio);
//  // xiaolu add 20180828 end
//
//  ec->DEnforce(denforceMap, agCmds, objlist, stripename, n, k, scratio, num, allnodes, ec->_locality);
////  ec->DEnforce(denforceMap, agCmds, objlist, stripename, n, k, scratio, num);
//
//  // 7. persists header
//  vector<AGCommand*> persistCmds;
//  ec->PersistHeader(agCmds, objlist, stripename, n, k, scratio, num, persistCmds);
//
//  // 8. send compute commands to distributor
//  vector<char*> todelete;
//  redisContext* distCtx = RedisUtil::createContext(_conf->_coorIp);
//
//  redisAppendCommand(distCtx, "MULTI");
//  for (auto item: agCmds) {
//    int cid = item.first;
//    AGCommand* agcmd = item.second;
//    unsigned int ip = agcmd->_sendIp;
//    ip = htonl(ip);
//    if (agcmd->_shouldSend) {
//      char* cmdstr = agcmd->_agCmd;
//      int cmLen = agcmd->_cmLen;
//      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
//      memcpy(todist, (char*)&ip, 4);
//      memcpy(todist+4, cmdstr, cmLen); 
//      todelete.push_back(todist);
//      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
//    }
//  }
//  for (auto agcmd: persistCmds) {
//    unsigned int ip = agcmd->_sendIp;
//    ip = htonl(ip);
//    if (agcmd->_shouldSend) {
//      char* cmdstr = agcmd->_agCmd;
//      int cmLen = agcmd->_cmLen;
//      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
//      memcpy(todist, (char*)&ip, 4);
//      memcpy(todist+4, cmdstr, cmLen); 
//      todelete.push_back(todist);
//      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
//    }
//  }
//  redisAppendCommand(distCtx, "EXEC");
//
//  redisReply* distReply;
//  redisGetReply(distCtx, (void **)&distReply);
//  freeReplyObject(distReply);
//  for (auto item: todelete) {
//    redisGetReply(distCtx, (void **)&distReply);
//    freeReplyObject(distReply);
//  }
//  redisGetReply(distCtx, (void **)&distReply);
//  freeReplyObject(distReply);
//  redisFree(distCtx);
//
//  // delete
//  if (ec) delete ec;
//  for (auto item: agCmds) delete item.second;
//  for (auto item: persistCmds) delete item;
//  for (auto item: todelete) free(item);
//}
//
//void Coordinator::recoveryOffline(string filename) {
//  cout << "Coordinator::recoveryOffline.repair " << filename << endl;
//
//  // 0. given the filename, get the ssentry
//  //    then get ecpool
//  SSEntry* ssentry = _stripeStore->getEntry(filename);
//  string poolname = ssentry->_poolname;
//  OfflineECPool* ecpool = _stripeStore->getECPool(poolname);
//  string stripename = ecpool->getStripeForObj(filename);
//  cout << "Coordinator::recoveryOffline.poolname: " << poolname << ", stripename: " << stripename << endl;
//
//  vector<string> stripeobjs = ecpool->getStripeObjList(stripename);
//  cout << "Coordinator::recoveryOffline.stripeobjs: ";
//  for (int i=0; i<stripeobjs.size(); i++) {
//    cout << stripeobjs[i] << ", ";
//  }
//  cout << endl;
//
//  // 1. figure out the corrupted sidx and integrity
//  int sid;
//  vector<int> integrity;
//  unordered_map<int, pair<string, unsigned int>> objlist;
//  int filesizeMB = 0;
//  vector<int> relocateIdx;  // we need to relocate the location for these index.
//  for (int i=0; i<stripeobjs.size(); i++) {
//    string curobj = stripeobjs[i];
//    if (filename.compare(curobj) == 0) {
//      sid = i;
//      integrity.push_back(0);
//      relocateIdx.push_back(i);
//    } else {
////      FSObjInputStream* curstream = new FSObjInputStream(_conf, curobj, _fs);
//      FSObjInputStream* curstream = new FSObjInputStream(_conf, curobj, _underfs);
//      if (curstream->exist()) integrity.push_back(1);
//      else integrity.push_back(0);
//      if  (curstream) delete curstream;
//    }
//    SSEntry* ssentry = _stripeStore->getEntry(curobj);
//    unsigned int loc = ssentry->_objLocs[0];
//    pair<string, unsigned int> curpair = make_pair(curobj, loc);
//    objlist.insert(make_pair(i, curpair));
//    if (ssentry->_filesizeMB > filesizeMB) filesizeMB = ssentry->_filesizeMB;
//  }
//
//  cout << "Coordinator::recoveryOffline.integrity:";
//  for (int i=0; i<integrity.size(); i++) {
//    cout << integrity[i] << " ";
//  }
//
//  // debug
//  cout << "Coordinator::recoveryOffline.objlist: ";
//  for (int i=0; i<stripeobjs.size(); i++) {
//    cout << "[" << i << ", " << objlist[i].first << ", " << RedisUtil::ip2Str(objlist[i].second) << "] ";
//  }
//  cout << endl;
//  cout << "Coordinator::recoveryOffline.relocateIdx: ";
//  for (int i=0; i<relocateIdx.size(); i++) cout << relocateIdx[i] << " ";
//  cout << endl;
//
//  ECPolicy* ecpolicy = ecpool->_ecpolicy;
//  ECBase* ec = ecpolicy->createECClass();
//
//  // xiaolu add 20180829 start
//  vector<vector<int>> group;
//  ec->place(group);
//  unordered_map<int, vector<int>> idx2group;
//  for (auto item: group) {
//    for (auto idx: item) {
//      idx2group.insert(make_pair(idx, item));
//    }
//  }
//  // xiaolu add 20180829 end
//
//  // relocate
//  sort(relocateIdx.begin(), relocateIdx.end());
//  int tmpi=0;
//  int curRelocateIdx=relocateIdx[tmpi];
//  vector<int> placed;
//  vector<unsigned int> ips;
//  for (int i=0; i<stripeobjs.size(); i++) {
//    if (i < curRelocateIdx) {
//      placed.push_back(i);
//      ips.push_back(objlist[i].second);
//    } else if (i == curRelocateIdx) {
//      vector<int> colocWith;
//      vector<int> notColocWith;
//
//      // xiaolu add 20180829 start
//      if (idx2group.find(i) != idx2group.end()) colocWith = idx2group[i];
//      if (colocWith.size() > 0) {
//        for (int j=0; j<ec->_n; j++) {
//          if (find(colocWith.begin(), colocWith.end(), j) == colocWith.end()) notColocWith.push_back(j);
//        }
//      }
//      // xiaolu add 20180829 end
//
//      // xiaolu comment 20180829 start
////      ec->placementRestriction(i, placed, colocWith, notColocWith);
//      // xiaolu comment 20180829 end
//
//      vector<unsigned int> candidates = getCandidates(ips, colocWith, notColocWith);
//      // we also need to remove the remaining nodes
//      for (int j=i+1; j<stripeobjs.size(); j++) {
//        if (find(relocateIdx.begin(), relocateIdx.end(), j) != relocateIdx.end()) continue;
//        else {
//          unsigned int toremove = objlist[j].second;
//          vector<unsigned int>::iterator position = find(candidates.begin(), candidates.end(), toremove);
//          if (position != candidates.end()) candidates.erase(position);
//        }
//      }
//      // now we choose a location from candidates
//      cout << "Coordinator::recoveryOffline.relocate for " << i << ", choose from candidates: ";
//      for (auto item: candidates) cout << RedisUtil::ip2Str(item) << " ";
//      cout << endl;
//      unsigned int curloc = chooseFromCandidates(candidates, _conf->_repair_policy, "repair");
//      cout << "Coordinator::recoveryOffline.relocate " << i << " with " << RedisUtil::ip2Str(curloc) << endl;
//      placed.push_back(i);
//      ips.push_back(curloc);
//      // set curloc in objlist
//      objlist[i].second = curloc;
//      // update SSEntry
//      ssentry->setOfflineLoc(curloc);
//      // update curRelocateIdx
//      tmpi++;
//      if (tmpi >= relocateIdx.size()) break;
//      else curRelocateIdx = relocateIdx[tmpi];
//    }
//  }
// 
//
//  // 2. figure out avail and torec list
////  ECPolicy* ecpolicy = ecpool->_ecpolicy;
////  ECBase* ec = ecpolicy->createECClass();
//  int k = ec->_k;
//  int n = ec->_n;
//  int scratio = ec->_cps;
//  cout << "Coordinator::recoveryOffline.k:" << k << ", n:" << n << ", scratio:" << scratio << endl;
//  vector<int> avail;
//  vector<int> torec;
//  for (int i=0; i<n; i++) {
//    for (int j=0; j<scratio; j++) {
//      int cid=i*scratio+j;
//      if (integrity[i]) avail.push_back(cid);
//      else torec.push_back(cid);
//    }
//  }
//  cout << "Coordinator::recoveryOffline.avail:";
//  for (int i=0; i<avail.size(); i++) cout << avail[i] << " ";
//  cout << endl;
//  cout << "Coordinator::recoveryOffline.torec:";
//  for (int i=0; i<torec.size(); i++) cout << torec[i] << " ";
//  cout << endl;
//
//  // 3. prepare decode ECDAG
//  ec->decode(avail, torec);
//  ec->dump();
//
//  // 4. DEnforce
//  int num = filesizeMB * 1048576 / _conf->_pktSize;
//  unordered_map<int, bool> denforceMap;
//  unordered_map<int, AGCommand*> agCmds;
//  vector<unsigned int> allnodes = _conf->_agentsIPs;
//  
//  // xiaolu add 20180828 start
//  ec->Optimize(ec->_opt, objlist, _conf->_ip2Rack, n, k, scratio);
//  // xiaolu add 20180828 end
//
//  ec->DEnforce(denforceMap, agCmds, objlist, stripename, n, k, scratio, num, allnodes, ec->_locality);
////  ec->DEnforce(denforceMap, agCmds, objlist, stripename, n, k, scratio, num);
//
//  // 5. Persist
//  vector<AGCommand*> persistCmds;
//  ec->PersistHeader(agCmds, objlist, stripename, n, k, scratio, num, persistCmds);
//
//  // 6. send compute commands to distributor
//  vector<char*> todelete;
//  redisContext* distCtx = RedisUtil::createContext(_conf->_coorIp);
//
//  redisAppendCommand(distCtx, "MULTI");
//  for (auto item: agCmds) {
//    int cid = item.first;
//    AGCommand* agcmd = item.second;
//    unsigned int ip = agcmd->_sendIp;
//    ip = htonl(ip);
//    if (agcmd->_shouldSend) {
//      char* cmdstr = agcmd->_agCmd;
//      int cmLen = agcmd->_cmLen;
//      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
//      memcpy(todist, (char*)&ip, 4);
//      memcpy(todist+4, cmdstr, cmLen); 
//      todelete.push_back(todist);
//      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
//    }
//  }
//  for (auto agcmd: persistCmds) {
//    unsigned int ip = agcmd->_sendIp;
//    ip = htonl(ip);
//    if (agcmd->_shouldSend) {
//      char* cmdstr = agcmd->_agCmd;
//      int cmLen = agcmd->_cmLen;
//      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
//      memcpy(todist, (char*)&ip, 4);
//      memcpy(todist+4, cmdstr, cmLen); 
//      todelete.push_back(todist);
//      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
//    }
//  }
//  redisAppendCommand(distCtx, "EXEC");
//
//  redisReply* distReply;
//  redisGetReply(distCtx, (void **)&distReply);
//  freeReplyObject(distReply);
//  for (auto item: todelete) {
//    redisGetReply(distCtx, (void **)&distReply);
//    freeReplyObject(distReply);
//  }
//  redisGetReply(distCtx, (void **)&distReply);
//  freeReplyObject(distReply);
//  redisFree(distCtx);
//
//
//  // last. delete
//  if (ec) delete ec;
//  for (auto item: agCmds) delete item.second;
//  for (auto item: persistCmds) delete item;
//  for (auto item: todelete) free(item);
//}
//
//void Coordinator::enableEncoding(CoorCommand* coorCmd) {
//  _stripeStore->setScan(true);
//}
//
//void Coordinator::enableRepair(CoorCommand* coorCmd) {
//  _stripeStore->setRepair(true);
//}
