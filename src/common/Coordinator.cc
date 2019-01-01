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
  _underfs = FSUtil::createFS(_conf->_fsType, _conf->_fsFactory[_conf->_fsType], _conf);
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
        case 3: getFileMeta(coorCmd); break;
        case 4: offlineEnc(coorCmd); break;
        case 5: offlineDegradedInst(coorCmd); break;
        case 6: reportLost(coorCmd); break;
        case 7: setECStatus(coorCmd); break;
        case 8: repairReqFromSS(coorCmd); break;
        case 9: onlineDegradedInst(coorCmd); break;
        case 11: reportRepaired(coorCmd); break;
        case 12: coorBenchmark(coorCmd); break;
        default: break;
      }
      delete coorCmd;
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
  cout << "Coordinator::registerOfflineEC" << endl;
  struct timeval time1, time2, time3, time4;
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

  // free
  delete ec;
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
  } else if (objname.find("oecstripe") != string::npos) {
    // placement request for an oec parity object
    assert(_stripeStore->existEntry(objname));
    SSEntry* ssentry = _stripeStore->getEntry(objname);
    vector<unsigned int> locs = ssentry->getObjloc();
    toret[0] = locs[0];
  } else {
    vector<unsigned int> candidates = _conf->_agentsIPs;
    toret[0] = chooseFromCandidates(candidates, "random", "other");
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
  if (type == 1) {
     // offline encoding
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

  // backup this ssentry
  _stripeStore->backupEntry(ssentry->toString());
}

void Coordinator::offlineEnc(CoorCommand* coorCmd) {
  string ecpoolid = coorCmd->getECPoolId();
  string stripename = coorCmd->getStripeName();
  cout << "Coordinator::offlineEnc start for " << stripename << endl; 

  // 0. given ecpoolid, get OfflineECPool
  OfflineECPool* ecpool = _stripeStore->getECPool(ecpoolid); 
  ecpool->lock();
  ECPolicy* ecpolicy = ecpool->getEcpolicy(); 
  ECBase* ec = ecpolicy->createECClass();
  int n = ecpolicy->getN();
  int k = ecpolicy->getK();
  int w = ecpolicy->getW(); 
  bool locality = ecpolicy->getLocality();
  int opt = ecpolicy->getOpt();

  vector<vector<int>> group;
  ec->Place(group);
  unordered_map<int, vector<int>> idx2group;
  for (auto item: group) {
    for (auto idx: item) {
      idx2group.insert(make_pair(idx, item));
    }
  }

  // 1. encode ecdag
  ECDAG* ecdag = ec->Encode();
  ecdag->reconstruct(opt);

  // 2. collect physical information
  // stripeidx -> {objname, location}
  unordered_map<int, pair<string, unsigned int>> objlist;
  // stripeidx -> location
  unordered_map<int, unsigned int> sid2ip;
  // objs in current stripe (now we only have source objs)
  vector<string> stripelist = ecpool->getStripeObjList(stripename); 
  // location for current stripe, indexed by stripe idx (now we only have source locations)
  vector<unsigned int> stripeips;
  // stripeplaced records the objnames that have been stored in this stripe
  vector<int> stripeplaced;
 
//  cout << "Coordinator::offlineEnc.encode stripe " << stripename << ", objlist: ";
//  for (int i=0; i<stripelist.size(); i++) cout << stripelist[i] << " ";
//  cout << endl;

  // maximum obj size in current stripe
  int basesizeMB = ecpool->getBasesize();
  int pktnum = basesizeMB * 1048576/_conf->_pktSize;
  
  // 2.1 get physical information for k source objs
  for (int i=0; i<stripelist.size(); i++) {
    int sid = i;
    string objname = stripelist[i];
    SSEntry* ssentry = _stripeStore->getEntryFromObj(objname); 
    unsigned int loc = ssentry->getLocOfObj(objname);
    pair<string, unsigned int> curpair = make_pair(objname, loc);

    objlist.insert(make_pair(sid, curpair));
    sid2ip.insert(make_pair(sid, loc));
    stripeips.push_back(loc);
    stripeplaced.push_back(i);
  }
  // 2.2 prepare physical information for m parity objs
  vector<string> parityobj;
  for (int i=k; i<n; i++) {
    string objname = "/"+ecpoolid+"-"+stripename+"-"+to_string(i);
    parityobj.push_back(objname);
    vector<int> colocWith;
    if (idx2group.find(i) != idx2group.end()) colocWith = idx2group[i];
    vector<unsigned int> candidates = getCandidates(stripeips, stripeplaced, colocWith);
    unsigned int loc = chooseFromCandidates(candidates, _conf->_data_policy, "data");
    pair<string, unsigned int> curpair = make_pair(objname, loc);

    objlist.insert(make_pair(i, curpair));
    sid2ip.insert(make_pair(i, loc));
    stripeips.push_back(loc);
    stripeplaced.push_back(i);

    // add parity obj to ecpool
    ecpool->addObj(objname, stripename);
    // create ssentry for parity obj, only has 1 obj in it
    SSEntry* ssentry = new SSEntry(objname, 1, basesizeMB, ecpoolid, {objname}, {loc});
    _stripeStore->insertEntry(ssentry);
  }
  ecpool->unlock();

  // debug info
//  for (auto item: objlist) {
//    int sid = item.first;
//    pair<string, unsigned int> pair = item.second;
//    string objname = pair.first;
//    unsigned int ip = pair.second;
//    cout << "stripe physical info: idx: " << sid << ", objname: " << objname << ", ip: " << RedisUtil::ip2Str(ip) << endl;
//  }
  
  // 4. topological sorting
  vector<int> sortedList = ecdag->toposort();

  // 5. figure out corresponding ip for corresponding node
  unordered_map<int, unsigned int> cid2ip;
  for (int i=0; i<sortedList.size(); i++) {
    int cidx = sortedList[i];
    ECNode* node = ecdag->getNode(cidx);
    vector<unsigned int> candidates = node->candidateIps(sid2ip, cid2ip, _conf->_agentsIPs, n, k, w, locality);
    // choose from candidates
    unsigned int curip = chooseFromCandidates(candidates, _conf->_encode_policy, "encode");
    cid2ip.insert(make_pair(cidx, curip));
  }

  // optimize
  ecdag->optimize2(opt, cid2ip, _conf->_ip2Rack, n, k, w, sid2ip, _conf->_agentsIPs, locality);
  ecdag->dump();

//  for (auto item: cid2ip) {
//    cout << "cid: " << item.first << ", ip: " << RedisUtil::ip2Str(item.second) << endl;
//  }

  // 6. parse for oec
  unordered_map<int, AGCommand*> agCmds = ecdag->parseForOEC(cid2ip, stripename, n, k, w, pktnum, objlist);

  // 7. add persist cmd
  vector<AGCommand*> persistCmds = ecdag->persist(cid2ip, stripename, n, k, w, pktnum, objlist);

  // 8. send commands to cmddistributor
  vector<char*> todelete;
  redisContext* distCtx = RedisUtil::createContext(_conf->_coorIp);

  redisAppendCommand(distCtx, "MULTI");
  for (auto item: agCmds) {
    auto agcmd = item.second;
    if (agcmd == NULL) continue;
    unsigned int ip = agcmd->getSendIp();
    ip = htonl(ip);
    if (agcmd->getShouldSend()) {
      char* cmdstr = agcmd->getCmd();
      int cmLen = agcmd->getCmdLen();
      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
      memcpy(todist, (char*)&ip, 4);
      memcpy(todist+4, cmdstr, cmLen); 
      todelete.push_back(todist);
      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
    }
  }
  for (auto agcmd: persistCmds) {
    if (agcmd == NULL) continue;
    unsigned int ip = agcmd->getSendIp();
    ip = htonl(ip);
    if (agcmd->getShouldSend()) {
      char* cmdstr = agcmd->getCmd();
      int cmLen = agcmd->getCmdLen();
      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
      memcpy(todist, (char*)&ip, 4);
      memcpy(todist+4, cmdstr, cmLen); 
      todelete.push_back(todist);
      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
    }
  }
  redisAppendCommand(distCtx, "EXEC");

  redisReply* distReply;
  redisGetReply(distCtx, (void **)&distReply);
  freeReplyObject(distReply);
  for (auto item: todelete) {
    redisGetReply(distCtx, (void **)&distReply);
    freeReplyObject(distReply);
  }
  redisGetReply(distCtx, (void **)&distReply);
  freeReplyObject(distReply);
  redisFree(distCtx);
  
  // 9. wait for finish flag?
  for (auto agcmd: persistCmds) {
    unsigned int ip = agcmd->getSendIp(); 
    redisContext* waitCtx = RedisUtil::createContext(ip);
    string wkey = "writefinish:"+agcmd->getWriteObjName();
    redisReply* fReply = (redisReply*)redisCommand(waitCtx, "blpop %s 0", wkey.c_str());
    freeReplyObject(fReply);
    redisFree(waitCtx);
  }
  cout << "Coordinator::offlineEnc for " << stripename << " finishes" << endl;
  _stripeStore->finishECStripe(ecpool, stripename);

  // backup entry for parity obj
  for (int i=0; i<parityobj.size(); i++) {
    SSEntry* curentry = _stripeStore->getEntryFromObj(parityobj[i]);
    _stripeStore->backupEntry(curentry->toString());
  }
  
  // free
  delete ecdag;
  delete ec; 
  for (auto item: agCmds) if (item.second) delete item.second;
  for (auto item: persistCmds) if (item) delete item;
  for (auto item: todelete) free(item);
}

void Coordinator::setECStatus(CoorCommand* coorCmd) {
  int op = coorCmd->getOp();
  string ectype = coorCmd->getECType();
  _stripeStore->setECStatus(op, ectype);
}

void Coordinator::getFileMeta(CoorCommand* coorCmd) {
  cout << "Coordinator::getFileMeta" << endl;
  struct timeval time1, time2;
  gettimeofday(&time1, NULL);
  string filename = coorCmd->getFilename();
  unsigned int clientip = coorCmd->getClientip();

  // online:  |type|filesizeMB|ecn|eck|ecw|
  // offline: |type|filesizeMB|objnum|

  // 0. getssentry
  SSEntry* ssentry = _stripeStore->getEntry(filename);
  assert(ssentry != NULL);
  int redundancy = ssentry->getType();
  int filesizeMB = ssentry->getFilesizeMB();
  // 1. type
  char* filemeta = (char*)calloc(1024, sizeof(char));
  string key = "filemeta:"+filename;
  int metaoff = 0;
  // 1.1 redundancy type
  int tmpr = htonl(redundancy);
  memcpy(filemeta + metaoff, (char*)&tmpr, 4); metaoff += 4;
  // 1.2 filesizeMB
  int tmpsize = htonl(filesizeMB);
  memcpy(filemeta + metaoff, (char*)&tmpsize, 4); metaoff += 4;

  if (redundancy == 0) {
    // for onlineec
    string ecid = ssentry->getEcidpool();
    ECPolicy* ecpolicy = _conf->_ecPolicyMap[ecid];
    ECBase* ec = ecpolicy->createECClass();
    int ecn = ecpolicy->getN();
    int eck = ecpolicy->getK();
    int ecw = ecpolicy->getW();
    // ecn|eck|ecw|
    int tmpn = htonl(ecn);
    memcpy(filemeta + metaoff, (char*)&tmpn, 4); metaoff += 4;
    int tmpk = htonl(eck);
    memcpy(filemeta + metaoff, (char*)&tmpk, 4); metaoff += 4;
    int tmpw = htonl(ecw);
    memcpy(filemeta + metaoff, (char*)&tmpw, 4); metaoff += 4;

    delete ec;
  } else {
    // for offlineec
    vector<string> objlist = ssentry->getObjlist();
    // append number of obj
    int numobjs=objlist.size();
    int tmpnum = htonl(numobjs);
    memcpy(filemeta + metaoff, (char*)&tmpnum, 4); metaoff += 4;
  }

  redisContext* sendCtx = RedisUtil::createContext(clientip);
  redisReply* rReply = (redisReply*)redisCommand(sendCtx, "RPUSH %s %b", key.c_str(), filemeta, metaoff);
  freeReplyObject(rReply);
  redisFree(sendCtx);
}

void Coordinator::onlineDegradedInst(CoorCommand* coorCmd) {
  cout << "Coordinator::onlineDegradedInst" << endl;
  struct timeval time1, time2, time3, time4;
  gettimeofday(&time1, NULL);
  string filename = coorCmd->getFilename();
  vector<int> corruptIdx = coorCmd->getCorruptIdx();
  unsigned int ip = coorCmd->getClientip();

  // |loadn|loadidx|computen|computetasks|
  // 1. create filemeta
  char* instruction = (char*)calloc(1024, sizeof(char));
  int offset = 0;

  // 2. create ssentry
  SSEntry* ssentry = _stripeStore->getEntry(filename);
  assert(ssentry != NULL);
  string ecid = ssentry->getEcidpool();
  ECPolicy* ecpolicy = _conf->_ecPolicyMap[ecid];
  ECBase* ec = ecpolicy->createECClass();
  int ecn = ecpolicy->getN();
  int eck = ecpolicy->getK();
  int ecw = ecpolicy->getW();

  // 3. integrity
  vector<int> integrity;
  for (int i=0; i<ecn; i++) {
    string objname = filename+"_oecobj_"+to_string(i);
    if (find(corruptIdx.begin(), corruptIdx.end(), i) != corruptIdx.end()) {
      integrity.push_back(0);
      _stripeStore->addLostObj(objname);
    } else {
      integrity.push_back(1);
    }
  }

  int loadn, computen;
  vector<ECTask*> computetasks;

  // 4. figure out avail and torec
  vector<int> availcidx;
  vector<int> toreccidx;
  for (int i=0; i<eck; i++) {
    if (integrity[i] == 1) {
      for (int j=0; j<ecw; j++) availcidx.push_back(i * ecw + j);
    } else {
      for (int j=0; j<ecw; j++) toreccidx.push_back(i * ecw + j);
    }
  }
  for (int i=eck; i<ecn; i++) {
    if (integrity[i] == 1) {
      for (int j=0; j<ecw; j++) availcidx.push_back(i * ecw + j);
    }
  }
  // obtain decode ecdag
  ECDAG* ecdag = ec->Decode(availcidx, toreccidx);
  vector<int> toposeq = ecdag->toposort();
  cout << "toposeq: ";
  for (int i=0; i<toposeq.size(); i++) cout << toposeq[i] << " ";
  cout << endl;
   
  // prepare for load tasks
  vector<int> leaves = ecdag->getLeaves();
  vector<int> loadidx;
  for (int i=0; i<leaves.size(); i++) {
    int sidx = leaves[i]/ecw;
    if (find(loadidx.begin(), loadidx.end(), sidx) == loadidx.end()) loadidx.push_back(sidx);
  }
  // as online degraded read need all source data, we need to check index less than k
  for (int i=0; i<eck; i++) {
    if (integrity[i] == 0) continue; 
    else if (find(loadidx.begin(), loadidx.end(), i) == loadidx.end()) loadidx.push_back(i);
  }
  sort(loadidx.begin(), loadidx.end());
  
  // loadn
  loadn = loadidx.size();
  int tmploadn = htonl(loadn);
  memcpy(instruction + offset, (char*)&tmploadn, 4); offset += 4;
  for (int i=0; i<loadn; i++) {
    int tmpidx = htonl(loadidx[i]);
    memcpy(instruction + offset, (char*)&tmpidx, 4); offset += 4;
  }
  // prepare for compute tasks 
  for (int i=0; i<toposeq.size(); i++) {
    ECNode* curnode = ecdag->getNode(toposeq[i]);
    curnode->parseForClient(computetasks);
  }
  for (int i=0; i<computetasks.size(); i++) computetasks[i]->dump();
  computen = computetasks.size();
  int tmpcomputen = htonl(computen);
  memcpy(instruction + offset, (char*)&tmpcomputen, 4); offset += 4;

  string key = "onlinedegradedinst:"+filename;
  redisContext* sendCtx = RedisUtil::createContext(ip);
  redisReply* rReply = (redisReply*)redisCommand(sendCtx, "RPUSH %s %b", key.c_str(), instruction, offset);
  freeReplyObject(rReply);
  redisFree(sendCtx);
  
  // send compute tasks
  for (int i=0; i<computetasks.size(); i++) {
    ECTask* curcompute = computetasks[i];
    curcompute->buildType2();
    string key = "compute:"+filename+":"+to_string(i);
    curcompute->sendTo(key, ip);
  }

  // delete
  for (int i=0; i<computetasks.size(); i++) delete computetasks[i];
  delete ecdag;
  delete ec;
  free(instruction);
}

void Coordinator::offlineDegradedInst(CoorCommand* coorCmd) {
  unsigned int clientIp = coorCmd->getClientip();
  string lostobj = coorCmd->getFilename();
  _stripeStore->addLostObj(lostobj);

  // 1. given lostobj, find SSEntry and figure out opt version
  SSEntry* ssentry = _stripeStore->getEntryFromObj(lostobj);
  string ecpoolid = ssentry->getEcidpool();
  OfflineECPool* ecpool = _stripeStore->getECPool(ecpoolid);
  ecpool->lock();
  ECPolicy* ecpolicy = ecpool->getEcpolicy();  
  int opt = ecpolicy->getOpt();

  if (opt < 0) {
    nonOptOfflineDegrade(lostobj, clientIp, ecpool, ecpolicy); 
    // 2. if opt version < 0, apply non-optimized degraded read
    // 2.1 in this case, we need ecdag, toposort and parseForClient
    // 2.2 we need object name and index that needs to be read from DSS
    // 2.3 we need lostidx computetasks n, k, w to compute lost pkt
    // return: lostidx|n|k|w|loadn|objname-objidx|objname-objidx|..|computen|computetasks|
  } else {
    optOfflineDegrade(lostobj, clientIp, ecpool, ecpolicy);
    // 3. if opt version >=0, apply optimized degraded read
    // 3.1 in this case, we need ecdag, toposort and parseForOEC, which requires cid2ip, stripename, n,k,w,pktnum,objlist
    // after we create commands, we send these commands to corresponding Agenst
    // we only need to tell client where to fetch and key to fetch
    // return: num|key-ip|key-ip|..|
  }
  ecpool->unlock();
}

void Coordinator::optOfflineDegrade(string lostobj, unsigned int clientIp, OfflineECPool* ecpool, ECPolicy* ecpolicy) {
  // return |opt|stripename|num|key-ip|key-ip|...|
  cout << "Coordinator::optOfflineDegrade" << endl;
  int opt = ecpolicy->getOpt();  

  // 0. create ec instances
  ECBase* ec = ecpolicy->createECClass();

  // 1, get stripeobjs for lostobj to figure out lostidx
  string stripename = ecpool->getStripeForObj(lostobj);
  vector<string> stripeobjs = ecpool->getStripeObjList(stripename);
  int lostidx;
  vector<int> integrity;
  for (int i=0; i<stripeobjs.size(); i++) {
    if (lostobj == stripeobjs[i]) {
      integrity.push_back(0);
      lostidx = i;
    } else {
      integrity.push_back(1);
    }
  }

  // prepare availcidx and toreccidx
  int ecn = ecpolicy->getN();
  int eck = ecpolicy->getK();
  int ecw = ecpolicy->getW();
  bool locality = ecpolicy->getLocality();
  vector<int> availcidx;
  vector<int> toreccidx;
  for (int i=0; i<ecn; i++) {
    if (i == lostidx) {
      for (int j=0; j<ecw; j++) toreccidx.push_back(i*ecw+j);
    } else {
      for (int j=0; j<ecw; j++) availcidx.push_back(i*ecw+j);
    }
  }

  // create ecdag
  ECDAG* ecdag = ec->Decode(availcidx, toreccidx);
  ecdag->reconstruct(opt);

  // prepare sid2ip, for cip2ip
  // prepare stripeips for client info
  unordered_map<int, unsigned int> sid2ip;
  unordered_map<int, pair<string, unsigned int>> objlist;
  vector<unsigned int> stripeips;
  for (int i=0; i<stripeobjs.size(); i++) {
    int sid = i;
    string objname = stripeobjs[i];
    SSEntry* ssentry = _stripeStore->getEntryFromObj(objname); 
    unsigned int loc;
    if (integrity[i] == 1) loc = ssentry->getLocOfObj(objname);
    else loc=clientIp;
    pair<string, unsigned int> curpair = make_pair(objname, loc);

    objlist.insert(make_pair(sid, curpair));
    sid2ip.insert(make_pair(sid, loc));
    stripeips.push_back(loc);
  }

  vector<int> toposeq = ecdag->toposort();
  ecdag->dump();

  // prepare cid2ip, for parseForOEC
  unordered_map<int, unsigned int> cid2ip;
  for (int i=0; i<toposeq.size(); i++) {
    int cidx = toposeq[i];
    ECNode* node = ecdag->getNode(cidx);
    vector<unsigned int> candidates = node->candidateIps(sid2ip, cid2ip, _conf->_agentsIPs, ecn, eck, ecw, locality);
    // choose from candidates
    unsigned int curip = chooseFromCandidates(candidates, _conf->_repair_policy, "repair");
    cid2ip.insert(make_pair(cidx, curip));
  }

  // prepare pktnum for parseForOEC
  int basesizeMB = ecpool->getBasesize();
  int pktnum = basesizeMB * 1048576/_conf->_pktSize;

  // optimize
  ecdag->optimize2(opt, cid2ip, _conf->_ip2Rack, ecn, eck, ecw, sid2ip, _conf->_agentsIPs, locality);

  // 6. parse for oec
  unordered_map<int, AGCommand*> agCmds = ecdag->parseForOEC(cid2ip, stripename, ecn, eck, ecw, pktnum, objlist);

  // 7. figure out roots and their ip
  vector<int> headers = ecdag->getHeaders();
  sort(headers.begin(), headers.end());
  int numblks = headers.size()/ecw; 
  // there should only be one blk for offline degraded read
  assert(numblks == 1);
  vector<pair<int, unsigned int>> rootinfo;
  for (int i=0; i<headers.size(); i++) {
    int cid = headers[i];
    //unsigned int loc = agCmds[cid]->getSendIp();
    unsigned int loc = ecdag->getNode(cid)->getIp();
    pair<int, unsigned int> curpair = make_pair(cid, loc);
    rootinfo.push_back(curpair);
  }
   
  // 8. send info to client
  char* instruction = (char*)calloc(1024,sizeof(char));
  int offset = 0; 

  // return |opt|stripename|num|key-ip|key-ip|...|
  int tmpopt = htonl(opt);
  memcpy(instruction + offset, (char*)&tmpopt, 4); offset += 4;
  int tmpstripenamelen = htonl(stripename.length());
  memcpy(instruction + offset, (char*)&tmpstripenamelen, 4); offset += 4;
  memcpy(instruction + offset, stripename.c_str(), stripename.length()); offset += stripename.length();
  int tmpnum = htonl(rootinfo.size());
  memcpy(instruction + offset, (char*)&tmpnum, 4); offset += 4;
  for (int i=0; i<rootinfo.size(); i++) {
    pair<int, unsigned int> curpair = rootinfo[i];
    int tmpcidx = htonl(curpair.first);
    unsigned int tmpip = htonl(curpair.second);
    memcpy(instruction + offset, (char*)&tmpcidx, 4); offset += 4;
    memcpy(instruction + offset, (char*)&tmpip, 4); offset += 4;
  }

  // send instruction back to client agent
  string key = "offlinedegradedinst:"+lostobj;
  redisContext* sendCtx = RedisUtil::createContext(clientIp);
  redisReply* rReply = (redisReply*)redisCommand(sendCtx, "RPUSH %s %b", key.c_str(), instruction, offset);
  freeReplyObject(rReply);
  redisFree(sendCtx);
   
  // send commands to cmddistributor
  vector<char*> todelete;
  redisContext* distCtx = RedisUtil::createContext(_conf->_coorIp);

  redisAppendCommand(distCtx, "MULTI");
  //for (auto agcmd: agCmds) {
  for (auto item: agCmds) {
    auto agcmd = item.second;
    if (agcmd == NULL) continue;
    unsigned int ip = agcmd->getSendIp();
    ip = htonl(ip);
    if (agcmd->getShouldSend()) {
      char* cmdstr = agcmd->getCmd();
      int cmLen = agcmd->getCmdLen();
      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
      memcpy(todist, (char*)&ip, 4);
      memcpy(todist+4, cmdstr, cmLen); 
      todelete.push_back(todist);
      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
    }
  }
  redisAppendCommand(distCtx, "EXEC");

  redisReply* distReply;
  redisGetReply(distCtx, (void **)&distReply);
  freeReplyObject(distReply);
  for (auto item: todelete) {
    redisGetReply(distCtx, (void **)&distReply);
    freeReplyObject(distReply);
  }
  redisGetReply(distCtx, (void **)&distReply);
  freeReplyObject(distReply);
  redisFree(distCtx);
  

  // delete
  delete ecdag;
  delete ec;
  for (auto item: agCmds) if(item.second) delete item.second;
  for (auto item: todelete) free(item);
}

void Coordinator::nonOptOfflineDegrade(string lostobj, unsigned int clientIp, OfflineECPool* ecpool, ECPolicy* ecpolicy) {
  cout << "Coordinator::nonOptOfflineDegrade" << endl;
  int opt = ecpolicy->getOpt(); 
  // 0, create ec instance
  ECBase* ec = ecpolicy->createECClass();

  // 1, get stripeobjs for lostobj to figure out lostidx
  string stripename = ecpool->getStripeForObj(lostobj);
  vector<string> stripeobjs = ecpool->getStripeObjList(stripename);
  int lostidx;
  vector<int> integrity;
  for (int i=0; i<stripeobjs.size(); i++) {
    if (lostobj == stripeobjs[i]) {
      integrity.push_back(0);
      lostidx = i;
    } else {
      integrity.push_back(1);
    }
  }
  cout << "Coordinator::nonOptOfflineDegrade.lostidx = " << lostidx << endl;

  // 2. we need n, k, w to obtain availcidx and toreccidx
  int ecn = ecpolicy->getN();
  int eck = ecpolicy->getK();
  int ecw = ecpolicy->getW();
  vector<int> availcidx;
  vector<int> toreccidx;
  for (int i=0; i<ecn; i++) {
    if (i == lostidx) {
      for (int j=0; j<ecw; j++) toreccidx.push_back(i*ecw+j);
    } else {
      for (int j=0; j<ecw; j++) availcidx.push_back(i*ecw+j);
    }
  }

  // need availcidx and toreccidx
  ECDAG* ecdag = ec->Decode(availcidx, toreccidx);
  vector<int> toposeq = ecdag->toposort();
 
  // obtain information for source objs
  vector<int> leaves = ecdag->getLeaves();
  vector<int> loadidx;
  vector<string> loadobjs;
  unordered_map<int, vector<int>> sid2Cids;
  for (int i=0; i<leaves.size(); i++) {
    int sidx = leaves[i]/ecw;
    if (sid2Cids.find(sidx) == sid2Cids.end()) {
      vector<int> curlist = {leaves[i]};
      sid2Cids.insert(make_pair(sidx, curlist));
      loadidx.push_back(sidx);
      loadobjs.push_back(stripeobjs[sidx]);
    } else {
      sid2Cids[sidx].push_back(leaves[i]);
    }
  }
  // obtain computetask
  vector<ECTask*> computetasks;
  for (int i=0; i<toposeq.size(); i++) {
    ECNode* curnode = ecdag->getNode(toposeq[i]);
    curnode->parseForClient(computetasks);
  }
  for (int i=0; i<computetasks.size(); i++) computetasks[i]->dump();

  char* instruction = (char*)calloc(1024,sizeof(char));
  int offset = 0; 

  // we need to return 
  // |opt|lostidx|ecn|eck|ecw|loadn|loadidx-objname|cidnum|cidxs|..|computen|computetask|..|
  int tmpopt = htonl(opt);
  memcpy(instruction + offset, (char*)&tmpopt, 4); offset += 4;
  int tmplostidx = htonl(lostidx);
  memcpy(instruction + offset, (char*)&tmplostidx, 4); offset += 4;
  int tmpecn = htonl(ecn);
  memcpy(instruction + offset, (char*)&tmpecn, 4); offset += 4;
  int tmpeck = htonl(eck);
  memcpy(instruction + offset, (char*)&tmpeck, 4); offset += 4;
  int tmpecw = htonl(ecw);
  memcpy(instruction + offset, (char*)&tmpecw, 4); offset += 4;
  int tmploadn = htonl(loadidx.size());
  memcpy(instruction + offset, (char*)&tmploadn, 4); offset += 4;
  for (int i=0; i<loadidx.size(); i++) {
    int tmpidx = htonl(loadidx[i]);
    memcpy(instruction + offset, (char*)&tmpidx, 4); offset += 4;
    // objname
    string loadobjname = loadobjs[i];
    int len = loadobjname.size();
    int tmpobjlen = htonl(len);
    memcpy(instruction + offset, (char*)&tmpobjlen, 4); offset += 4;
    memcpy(instruction + offset, loadobjname.c_str(), len); offset += len;
    vector<int> curlist = sid2Cids[loadidx[i]];
    // num cids
    int numcids = curlist.size();
    int tmpnumcids = htonl(numcids);
    memcpy(instruction + offset, (char*)&tmpnumcids, 4); offset += 4;
    for (int j=0; j<numcids; j++) {
      int curcid = curlist[j];
      int tmpcid = htonl(curcid);
      memcpy(instruction + offset, (char*)&tmpcid, 4); offset += 4;
    }
  }
  int computen = computetasks.size();
  int tmpcomputen = htonl(computen);
  memcpy(instruction + offset, (char*)&tmpcomputen, 4); offset += 4;
 
  // first send out info without compute
  string key = "offlinedegradedinst:"+lostobj;
  redisContext* sendCtx = RedisUtil::createContext(clientIp);
  redisReply* rReply = (redisReply*)redisCommand(sendCtx, "RPUSH %s %b", key.c_str(), instruction, offset);
  freeReplyObject(rReply);
  redisFree(sendCtx);

  // then send out computetasks
  for (int i=0; i<computetasks.size(); i++) {
    ECTask* curcompute = computetasks[i];
    curcompute->buildType2();
    string key = "compute:"+lostobj+":"+to_string(i);
    curcompute->sendTo(key, clientIp);
  }

  // free
  for (auto task: computetasks) delete task;
  delete ecdag;
  delete ec;
  free(instruction);
}

void Coordinator::reportLost(CoorCommand* coorCmd) {
  string lostobj = coorCmd->getFilename();
  _stripeStore->addLostObj(lostobj);
}

void Coordinator::reportRepaired(CoorCommand* coorCmd) {
  string objname = coorCmd->getFilename();
  cout << "Coordinator::reportRepaired for " << objname << endl;
  _stripeStore->finishRepair(objname);
}

void Coordinator::repairReqFromSS(CoorCommand* coorCmd) {
  string objname = coorCmd->getFilename();
  cout << "Coordinator::repairReqFromSS.repair request for " << objname << endl;

  // figure out ec type
  SSEntry* ssentry = _stripeStore->getEntryFromObj(objname);
  int redundancy = ssentry->getType();

  if (redundancy == 0) {
    return recoveryOnline(objname);
  } else {
    return recoveryOffline(objname);
  }
}

void Coordinator::recoveryOnline(string lostobj) {
  // we need ecdag, toposort and parseForOEC, which requires cid2ip, stripename, n,k,w,pktnum,objlist
  // we also need to create persist command to persist repaired block
  // after we create commands, we send these commands to corresponding Agenst

  // 0. given lostobj, find SSEntry and figure out opt version
  SSEntry* ssentry = _stripeStore->getEntryFromObj(lostobj);
  string ecid = ssentry->getEcidpool();
  ECPolicy* ecpolicy = _conf->_ecPolicyMap[ecid];

  // 1. create ec instances
  ECBase* ec = ecpolicy->createECClass();

  // 2, get stripeobjs for lostobj to figure out lostidx
  string filename = ssentry->getFilename();
  string stripename = filename; 
  vector<string> stripeobjs = ssentry->getObjlist();
  int lostidx;
  vector<int> integrity;
  for (int i=0; i<stripeobjs.size(); i++) {
    if (lostobj == stripeobjs[i]) {
      integrity.push_back(0);
      lostidx = i;
    } else {
      integrity.push_back(1);
    }
  }

  // prepare availcidx and toreccidx
  int ecn = ecpolicy->getN();
  int eck = ecpolicy->getK();
  int ecw = ecpolicy->getW();
  bool locality = ecpolicy->getLocality();
  int opt = ecpolicy->getOpt();
  vector<int> availcidx;
  vector<int> toreccidx;
  for (int i=0; i<ecn; i++) {
    if (i == lostidx) {
      for (int j=0; j<ecw; j++) toreccidx.push_back(i*ecw+j);
    } else {
      for (int j=0; j<ecw; j++) availcidx.push_back(i*ecw+j);
    }
  }

  // create ecdag
  ECDAG* ecdag = ec->Decode(availcidx, toreccidx);
  ecdag->reconstruct(opt);

  // prepare sid2ip, for cip2ip
  // prepare stripeips for client info
  unordered_map<int, unsigned int> sid2ip;
  unordered_map<int, pair<string, unsigned int>> objlist;
  vector<unsigned int> stripeips;
  for (int i=0; i<stripeobjs.size(); i++) {
    int sid = i;
    string objname = stripeobjs[i];
    SSEntry* curssentry = _stripeStore->getEntryFromObj(objname); 
    unsigned int loc;
    if (integrity[i] == 1) loc = curssentry->getLocOfObj(objname);
    else loc=0;
    pair<string, unsigned int> curpair = make_pair(objname, loc);

    objlist.insert(make_pair(sid, curpair));
    sid2ip.insert(make_pair(sid, loc));
    stripeips.push_back(loc);
  }

  // we need to update the location for lostobj
  vector<vector<int>> group;
  ec->Place(group);
  // sort group to a map, such that we can find corresponding group based on idx
  unordered_map<int, vector<int>> idx2group;
  for (auto item: group) {
    for (auto idx: item) {
      idx2group.insert(make_pair(idx, item));
    }
  }
  // relocate 
  vector<unsigned int> placedIps;
  vector<int> placedIdx;
  for (int i=0; i<ecn; i++) {
    if (integrity[i] == 1) { 
      placedIdx.push_back(i);
      placedIps.push_back(stripeips[i]);
    } else {
      vector<int> colocWith;
      if (idx2group.find(i) != idx2group.end()) colocWith = idx2group[i];
      vector<unsigned int> candidates = getCandidates(placedIps, placedIdx, colocWith);
      // we need to remove remaining ips in candidates
      for (int j=i+1; j<ecn; j++) {
        if (integrity[j] == 1) {
          unsigned int toremove = stripeips[j];
          vector<unsigned int>::iterator position = find(candidates.begin(), candidates.end(), toremove);
          if (position != candidates.end()) candidates.erase(position);
        }
      }
      // now we choose a loc from candidates
      unsigned int curip = chooseFromCandidates(candidates, _conf->_repair_policy, "repair");
      // update placedIps, placedIdx
      placedIps.push_back(curip);
      placedIdx.push_back(i);
      // update sid2ip, objlist, stripeips
      sid2ip[i] = curip;
      objlist[i].second = curip;
      stripeips[i] = curip;
      // we need to update the location in SSEntry
      string objname = objlist[i].first;
      SSEntry* curssentry = _stripeStore->getEntryFromObj(objname);
      curssentry->updateObjLoc(objname, curip);
    }
  }

  vector<int> toposeq = ecdag->toposort();

  // prepare cid2ip, for parseForOEC
  unordered_map<int, unsigned int> cid2ip;
  for (int i=0; i<toposeq.size(); i++) {
    int cidx = toposeq[i];
    ECNode* node = ecdag->getNode(cidx);
    vector<unsigned int> candidates = node->candidateIps(sid2ip, cid2ip, _conf->_agentsIPs, ecn, eck, ecw, locality, lostidx);
    // choose from candidates
    unsigned int curip = chooseFromCandidates(candidates, _conf->_repair_policy, "repair");
    cid2ip.insert(make_pair(cidx, curip));
  }

  int filesizeMB = ssentry->getFilesizeMB();
  int objsizeMB = filesizeMB / eck;
  int pktnum = objsizeMB * 1048576/_conf->_pktSize;

  // optimize
  ecdag->optimize2(opt, cid2ip, _conf->_ip2Rack, ecn, eck, ecw, sid2ip, _conf->_agentsIPs, locality);

  // 6. parse for oec
  //vector<AGCommand*> agCmds = ecdag->parseForOEC(cid2ip, stripename, ecn, eck, ecw, pktnum, objlist);
  unordered_map<int, AGCommand*> agCmds = ecdag->parseForOEC(cid2ip, stripename, ecn, eck, ecw, pktnum, objlist);
  
  // 7. add persist cmd
  vector<AGCommand*> persistCmds = ecdag->persist(cid2ip, stripename, ecn, eck, ecw, pktnum, objlist);
  
  // 8. send commands to cmddistributor
  vector<char*> todelete;
  redisContext* distCtx = RedisUtil::createContext(_conf->_coorIp);

  redisAppendCommand(distCtx, "MULTI");
  for (auto item: agCmds) {
    auto agcmd = item.second;
    if (agcmd == NULL) continue;
    unsigned int ip = agcmd->getSendIp();
    ip = htonl(ip);
    if (agcmd->getShouldSend()) {
      char* cmdstr = agcmd->getCmd();
      int cmLen = agcmd->getCmdLen();
      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
      memcpy(todist, (char*)&ip, 4);
      memcpy(todist+4, cmdstr, cmLen); 
      todelete.push_back(todist);
      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
    }
  }
  for (auto agcmd: persistCmds) {
    unsigned int ip = agcmd->getSendIp();
    ip = htonl(ip);
    if (agcmd->getShouldSend()) {
      char* cmdstr = agcmd->getCmd();
      int cmLen = agcmd->getCmdLen();
      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
      memcpy(todist, (char*)&ip, 4);
      memcpy(todist+4, cmdstr, cmLen); 
      todelete.push_back(todist);
      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
    }
  }
  redisAppendCommand(distCtx, "EXEC");

  redisReply* distReply;
  redisGetReply(distCtx, (void **)&distReply);
  freeReplyObject(distReply);
  for (auto item: todelete) {
    redisGetReply(distCtx, (void **)&distReply);
    freeReplyObject(distReply);
  }
  redisGetReply(distCtx, (void **)&distReply);
  freeReplyObject(distReply);
  redisFree(distCtx);

  // 9. wait for finish flag?
  for (auto agcmd: persistCmds) {
    unsigned int ip = agcmd->getSendIp(); 
    redisContext* waitCtx = RedisUtil::createContext(ip);
    string wkey = "writefinish:"+agcmd->getWriteObjName();
    redisReply* fReply = (redisReply*)redisCommand(waitCtx, "blpop %s 0", wkey.c_str());
    freeReplyObject(fReply);
    redisFree(waitCtx);
  }
  cout << "Coordinator::repair for " << lostobj << " finishes" << endl;

  // delete
  delete ec;
  delete ecdag;
  for (auto item: agCmds) if(item.second) delete item.second;
  for (auto item: persistCmds) if(item) delete item;
  for (auto item: todelete) free(item);
}

void Coordinator::recoveryOffline(string lostobj) {
  // obtain needed information
  SSEntry* ssentry = _stripeStore->getEntryFromObj(lostobj);
  string ecpoolid = ssentry->getEcidpool();
  OfflineECPool* ecpool = _stripeStore->getECPool(ecpoolid);
  ecpool->lock();
  ECPolicy* ecpolicy = ecpool->getEcpolicy();  

  // 1. create ec instances
  ECBase* ec = ecpolicy->createECClass();

  // 2, get stripeobjs for lostobj to figure out lostidx
  string stripename = ecpool->getStripeForObj(lostobj);
  vector<string> stripeobjs = ecpool->getStripeObjList(stripename);
  ecpool->unlock();
  int lostidx;
  vector<int> integrity;
  for (int i=0; i<stripeobjs.size(); i++) {
    if (lostobj == stripeobjs[i]) {
      integrity.push_back(0);
      lostidx = i;
    } else {
      integrity.push_back(1);
    }
  }

  // prepare availcidx and toreccidx
  int ecn = ecpolicy->getN();
  int eck = ecpolicy->getK();
  int ecw = ecpolicy->getW();
  bool locality = ecpolicy->getLocality();
  int opt = ecpolicy->getOpt();
  vector<int> availcidx;
  vector<int> toreccidx;
  for (int i=0; i<ecn; i++) {
    if (i == lostidx) {
      for (int j=0; j<ecw; j++) toreccidx.push_back(i*ecw+j);
    } else {
      for (int j=0; j<ecw; j++) availcidx.push_back(i*ecw+j);
    }
  }

  // create ecdag
  ECDAG* ecdag = ec->Decode(availcidx, toreccidx);
  ecdag->reconstruct(opt);

  // prepare sid2ip, for cip2ip
  // prepare stripeips for client info
  unordered_map<int, unsigned int> sid2ip;
  unordered_map<int, pair<string, unsigned int>> objlist;
  vector<unsigned int> stripeips;
  for (int i=0; i<stripeobjs.size(); i++) {
    int sid = i;
    string objname = stripeobjs[i];
    SSEntry* curssentry = _stripeStore->getEntryFromObj(objname); 
    unsigned int loc;
    if (integrity[i] == 1) loc = curssentry->getLocOfObj(objname);
    else loc=0;
    pair<string, unsigned int> curpair = make_pair(objname, loc);

    objlist.insert(make_pair(sid, curpair));
    sid2ip.insert(make_pair(sid, loc));
    stripeips.push_back(loc);
  }

  // we need to update the location for lostobj
  vector<vector<int>> group;
  ec->Place(group);
  // sort group to a map, such that we can find corresponding group based on idx
  unordered_map<int, vector<int>> idx2group;
  for (auto item: group) {
    for (auto idx: item) {
      idx2group.insert(make_pair(idx, item));
    }
  }
  // relocate 
  vector<unsigned int> placedIps;
  vector<int> placedIdx;
  for (int i=0; i<ecn; i++) {
    if (integrity[i] == 1) { 
      placedIdx.push_back(i);
      placedIps.push_back(stripeips[i]);
    } else {
      vector<int> colocWith;
      if (idx2group.find(i) != idx2group.end()) colocWith = idx2group[i];
      vector<unsigned int> candidates = getCandidates(placedIps, placedIdx, colocWith);
      // we need to remove remaining ips in candidates
      for (int j=i+1; j<ecn; j++) {
        if (integrity[j] == 1) {
          unsigned int toremove = stripeips[j];
          vector<unsigned int>::iterator position = find(candidates.begin(), candidates.end(), toremove);
          if (position != candidates.end()) candidates.erase(position);
        }
      }
      // now we choose a loc from candidates
      unsigned int curip = chooseFromCandidates(candidates, _conf->_repair_policy, "repair");
      // update placedIps, placedIdx
      placedIps.push_back(curip);
      placedIdx.push_back(i);
      // update sid2ip, objlist, stripeips
      sid2ip[i] = curip;
      objlist[i].second = curip;
      stripeips[i] = curip;
      // we need to update the location in SSEntry
      string objname = objlist[i].first;
      SSEntry* curssentry = _stripeStore->getEntryFromObj(objname);
      curssentry->updateObjLoc(objname, curip);
    }
  }
 
  vector<int> toposeq = ecdag->toposort();

  // prepare cid2ip, for parseForOEC
  unordered_map<int, unsigned int> cid2ip;
  for (int i=0; i<toposeq.size(); i++) {
    int cidx = toposeq[i];
    ECNode* node = ecdag->getNode(cidx);
    vector<unsigned int> candidates = node->candidateIps(sid2ip, cid2ip, _conf->_agentsIPs, ecn, eck, ecw, locality, lostidx);
    // choose from candidates
    unsigned int curip = chooseFromCandidates(candidates, _conf->_repair_policy, "repair");
    cid2ip.insert(make_pair(cidx, curip));
  }

  int filesizeMB = ssentry->getFilesizeMB();
  int objsizeMB = filesizeMB / eck;
  int pktnum = objsizeMB * 1048576/_conf->_pktSize;

  // optimize 
  ecdag->optimize2(opt, cid2ip, _conf->_ip2Rack, ecn, eck, ecw, sid2ip, _conf->_agentsIPs, locality);

  // 6. parse for oec
  //vector<AGCommand*> agCmds = ecdag->parseForOEC(cid2ip, stripename, ecn, eck, ecw, pktnum, objlist);
  unordered_map<int, AGCommand*> agCmds = ecdag->parseForOEC(cid2ip, stripename, ecn, eck, ecw, pktnum, objlist);
  
  // 7. add persist cmd
  vector<AGCommand*> persistCmds = ecdag->persist(cid2ip, stripename, ecn, eck, ecw, pktnum, objlist);
  
  // 8. send commands to cmddistributor
  vector<char*> todelete;
  redisContext* distCtx = RedisUtil::createContext(_conf->_coorIp);

  redisAppendCommand(distCtx, "MULTI");
  for (auto item: agCmds) {
    auto agcmd = item.second;
    if (agcmd == NULL) continue;
    unsigned int ip = agcmd->getSendIp();
    ip = htonl(ip);
    if (agcmd->getShouldSend()) {
      char* cmdstr = agcmd->getCmd();
      int cmLen = agcmd->getCmdLen();
      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
      memcpy(todist, (char*)&ip, 4);
      memcpy(todist+4, cmdstr, cmLen); 
      todelete.push_back(todist);
      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
    }
  }
  for (auto agcmd: persistCmds) {
    if (agcmd == NULL) continue;
    unsigned int ip = agcmd->getSendIp();
    ip = htonl(ip);
    if (agcmd->getShouldSend()) {
      char* cmdstr = agcmd->getCmd();
      int cmLen = agcmd->getCmdLen();
      char* todist = (char*)calloc(cmLen + 4, sizeof(char));
      memcpy(todist, (char*)&ip, 4);
      memcpy(todist+4, cmdstr, cmLen); 
      todelete.push_back(todist);
      redisAppendCommand(distCtx, "RPUSH dist_request %b", todist, cmLen+4);
    }
  }
  redisAppendCommand(distCtx, "EXEC");

  redisReply* distReply;
  redisGetReply(distCtx, (void **)&distReply);
  freeReplyObject(distReply);
  for (auto item: todelete) {
    redisGetReply(distCtx, (void **)&distReply);
    freeReplyObject(distReply);
  }
  redisGetReply(distCtx, (void **)&distReply);
  freeReplyObject(distReply);
  redisFree(distCtx);

  // 9. wait for finish flag?
  for (auto agcmd: persistCmds) {
    unsigned int ip = agcmd->getSendIp(); 
    redisContext* waitCtx = RedisUtil::createContext(ip);
    string wkey = "writefinish:"+agcmd->getWriteObjName();
    redisReply* fReply = (redisReply*)redisCommand(waitCtx, "blpop %s 0", wkey.c_str());
    freeReplyObject(fReply);
    redisFree(waitCtx);
  }
  cout << "Coordinator::repair for " << lostobj << " finishes" << endl;

  // delete
  delete ec;
  delete ecdag;
  for (auto item: agCmds) if (item.second) delete item.second;
  for (auto item: persistCmds) if (item) delete item;
  for (auto item: todelete) free(item);
}

void Coordinator::coorBenchmark(CoorCommand* coorCmd) {
  string benchname = coorCmd->getBenchName();
  unsigned int clientIp = coorCmd->getClientip();
  // plan to do simple ECDAG parsing in benchmark
  string ecid="rs_9_6";
  ECPolicy* ecpolicy = _conf->_ecPolicyMap[ecid];
  ECBase* ec=ecpolicy->createECClass();
  int ecn = ecpolicy->getN();
  int eck = ecpolicy->getK();
  int ecw = ecpolicy->getW();
  int opt = ecpolicy->getOpt();
  bool locality = ecpolicy->getLocality();

  ECDAG* ecdag;
  ecdag = ec->Encode();
  // first optimize without physical information
  ecdag->reconstruct(opt);

  // simulate physical information
  unordered_map<int, pair<string, unsigned int>> objlist;
  unordered_map<int, unsigned int> sid2ip;
  for (int sid=0; sid<ecn; sid++) {
    string objname = "testobj"+to_string(sid);
    unsigned int ip = _conf->_agentsIPs[sid];
    objlist.insert(make_pair(sid, make_pair(objname, ip)));
    sid2ip.insert(make_pair(sid, ip));
  }

  // topological sorting
  vector<int> toposeq = ecdag->toposort();

  // cid2ip
  unordered_map<int, unsigned int> cid2ip;
  for (int i=0; i<toposeq.size(); i++) {
    int curcid = toposeq[i];
    ECNode* cnode = ecdag->getNode(curcid);
    vector<unsigned int> candidates = cnode->candidateIps(sid2ip, cid2ip, _conf->_agentsIPs, ecn, eck, ecw, locality || (opt>0));
    unsigned int ip = candidates[0];
    cid2ip.insert(make_pair(curcid, ip));
  }

  ecdag->optimize2(opt, cid2ip, _conf->_ip2Rack, ecn, eck, ecw, sid2ip, _conf->_agentsIPs, locality || (opt>0));
  ecdag->dump();

  for (auto item: cid2ip) {
//    cout << "cid: " << item.first << ", ip: " << RedisUtil::ip2Str(item.second) << ", ";
    ECNode* cnode = ecdag->getNode(item.first);
    unordered_map<int, int> map = cnode->getRefMap();
//    for (auto iitem: map) {
//      cout << "ref["<<iitem.first<<"]: " << iitem.second << ", ";
//    }
//    cout << endl;
  }

  string stripename = "teststripe";
  int pktnum = 8;
  unordered_map<int, AGCommand*> agCmds = ecdag->parseForOEC(cid2ip, stripename, ecn, eck, ecw, pktnum, objlist);
  vector<AGCommand*> persistCmds = ecdag->persist(cid2ip, stripename, ecn, eck, ecw, pktnum, objlist); 

  // send back response to client
  // benchfinish:benchname
  redisReply* rReply;
  redisContext* waitCtx = RedisUtil::createContext(clientIp);
  string wkey = "benchfinish:" + benchname;
  int tmpval = htonl(1);
  rReply = (redisReply*)redisCommand(waitCtx, "rpush %s %b", wkey.c_str(), (char*)&tmpval, sizeof(tmpval));
  freeReplyObject(rReply);
  redisFree(waitCtx);

  // free
  for (auto item: persistCmds) delete item;
  for (auto item: agCmds) delete item.second;
  delete ecdag;
  delete ec;
}
