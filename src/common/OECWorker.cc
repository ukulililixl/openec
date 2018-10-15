#include "OECWorker.hh"

OECWorker::OECWorker(Config* conf) : _conf(conf) {
  // create local context
  try {
    _processCtx = RedisUtil::createContext(_conf -> _localIp);
    _localCtx = RedisUtil::createContext(_conf -> _localIp);
    _coorCtx = RedisUtil::createContext(_conf -> _coorIp);
  } catch (int e) {
    // TODO: error handling
    cerr << "initializing redis context error" << endl;
  }

//  _underfs = FSUtil::createFS(_conf->_fsType, _conf->_fsFactory[_conf->_fsType], _conf);
}

OECWorker::~OECWorker() {
  redisFree(_localCtx);
  redisFree(_processCtx);
  redisFree(_coorCtx);
//  delete _underfs;
}

void OECWorker::doProcess() {
  redisReply* rReply;
  while (true) {
//    cout << "OECWorker::doProcess" << endl;  
//    // will never stop looping
//    rReply = (redisReply*)redisCommand(_processCtx, "blpop ag_request 0");
//    if (rReply -> type == REDIS_REPLY_NIL) {
//      cerr << "OECWorker::doProcess() get feed back empty queue " << endl;
//      freeReplyObject(rReply);
//    } else if (rReply -> type == REDIS_REPLY_ERROR) {
//      cerr << "OECWorker::doProcess() get feed back ERROR happens " << endl;
//    } else {
//      struct timeval time1, time2;
//      gettimeofday(&time1, NULL);
//      char* reqStr = rReply -> element[1] -> str;
//      AGCommand* agCmd = new AGCommand(reqStr);
//      cout << "OECWorker::doProcess() receive a request of type " << agCmd->_type << endl;
//      agCmd->dumpCmd();
//      switch (agCmd->_type) {
//        case 0: clientWrite(agCmd); break;
//        case 1: clientRead(agCmd); break;
//        case 2: readDisk(agCmd); break;
//        case 3: fetchCompute(agCmd); break;
//        case 5: persist(agCmd); break;
//        case 6: readDiskList(agCmd); break;
//        case 7: readFetchCompute(agCmd); break;
//        default:break;
//      }
//      gettimeofday(&time2, NULL);
//      cout << "OECWorker::doProcess().duration = " << RedisUtil::duration(time1, time2) << endl;
//      // delete agCmd
//      delete agCmd;
//    }
//    // free reply object
//    freeReplyObject(rReply); 
  }
}

//void OECWorker::readDisk(AGCommand* agcmd) {
//  cout << "OECWorker::readDisk" << endl;
//  string stripename = agcmd->_stripeName;
//  int scratio = agcmd->_scratio;
//  int num = agcmd->_num;
//  string objname = agcmd->_readObjName;
//  int cid = agcmd->_Cid;
//
//  int pktsize = _conf->_pktSize;
//  int slicesize = pktsize/scratio;
//
//  string key = stripename+":"+to_string(cid);
//  int unitidx = cid%scratio;
//  cout << "OECWorker::readDisk. read." << objname << ", with " << num << " reads of size " << slicesize << " in " << pktsize << ", writekey = " << key << endl;
//
//  FSObjInputStream* objstream = new FSObjInputStream(_conf, objname, _underfs);
//  if (!objstream->exist()) {
//    cout << "OECWorker::readWorker." << objname << " does not exist!" << endl;
//    return;
//  }
//  thread readThread = thread([=]{objstream->readObj(slicesize, unitidx);});
//
//  BlockingQueue<OECDataPacket*>* readQueue = objstream->getQueue();
//  thread wWorker([=]{writeWorker(readQueue, key, num);});
//
//  // join
//  wWorker.join();
//  readThread.join();
//  
//  // delete
//  if (objstream) delete objstream;
//  cout << "OECWorker::readDisk finishes!" << endl;
//}
//
//void OECWorker::readDiskList(AGCommand* agcmd) {
//  cout << "OECWorker::readDiskList" << endl;
//  string stripename = agcmd->_stripeName;
//  int scratio = agcmd->_scratio;
//  int num = agcmd->_num;  // this num is pkt num, when scratio>1, a pkt is divided into several small sub-pkts.
//                          // the basic read unit is packet
//  string objname = agcmd->_readObjName;
//  vector<int> cidlist = agcmd->_readCidList;
//  sort(cidlist.begin(), cidlist.end());
//
//  int pktsize = _conf->_pktSize;
//  int slicesize = pktsize/scratio;
//  int numThreads = cidlist.size();
//  FSObjInputStream* objstream = new FSObjInputStream(_conf, objname, _underfs);
//  if (!objstream->exist()) {
//    cout << "OECWorker::readWorker." << objname << " does not exist!" << endl;
//    return;
//  }
//  // read data in serial from disk
//  thread readThread = thread([=]{objstream->readObj(slicesize);});
//  BlockingQueue<OECDataPacket*>* readQueue = objstream->getQueue();
//  // writeThread
//  thread writeThread = thread([=]{selectWriteWorker(readQueue, num, stripename, scratio, cidlist);});
//
//  //join
//  readThread.join();
//  writeThread.join();
//
//  // delete
//  if (objstream) delete objstream;
//  cout << "OECWorker::readDisk finishes!" << endl;
//}
//
//void OECWorker::selectWriteWorker(BlockingQueue<OECDataPacket*>* writeQueue,
//                           int pktnum,
//                           string keybase,
//                           int scratio,
//                           vector<int> idxlist) {
//  redisContext* writeCtx = RedisUtil::createContext(_conf->_localIp);
//  redisReply* rReply;
//
//  vector<int> units;
//  for (int i=0; i<idxlist.size(); i++) units.push_back(idxlist[i]%scratio);
//  struct timeval time1, time2;
//  gettimeofday(&time1, NULL);
//
//  int replyid=0;
//  int count=0;
//  for (int i=0; i<pktnum; i++) {
//    for (int j=0; j<scratio; j++) {
//      OECDataPacket* curslice = writeQueue->pop();
//      if (find(units.begin(), units.end(), j) == units.end()) {
//        delete curslice;
//        continue;
//      }
//      string key = keybase+":"+to_string(idxlist[j])+":"+to_string(i);
//      // we write data into redis
//      int len = curslice->_dataLen; 
//      char* raw = curslice->_raw;
//      int rawlen = len + 4;
//      redisAppendCommand(writeCtx, "RPUSH %s %b", key.c_str(), raw, rawlen); count++;
//      delete curslice;
//      if (i>1) {
//        redisGetReply(writeCtx, (void**)&rReply); replyid++;
//        freeReplyObject(rReply);
//      }
//    }
//  }
//
//  for (int i=replyid; i<count; i++)  {
//    redisGetReply(writeCtx, (void**)&rReply); replyid++;
//    freeReplyObject(rReply);
//  }
//
//  gettimeofday(&time2, NULL);
//  cout << "OECWorker::selectWriteWorker.duration: " << RedisUtil::duration(time1, time2) << " for " << keybase << endl;
//  redisFree(writeCtx);
//}
//
//void OECWorker::readFetchCompute(AGCommand* agcmd) {
//  cout << "OECWorker::readFetchCompute" << endl;
//  string stripename = agcmd->_stripeName;
//  int scratio = agcmd->_scratio;
//  int num = agcmd->_num;
//  string objname = agcmd->_readObjName;
//  int cid = agcmd->_Cid;
//
//  int nprevs = agcmd->_nprevs;
//  cout << "OECWorker::fetchCompute.nprev = " << nprevs << endl;
//  vector<int> prevcids = agcmd->_prevCids;
//  vector<unsigned int> prevlocs = agcmd->_prevLocs;
//  unordered_map<int, vector<int>> coefs = agcmd->_coefs;
//  vector<int> computefor;
//  for (auto item:coefs) {
//    computefor.push_back(item.first);
//  }
//  for (int i=0; i<nprevs; i++) {
//    int cid = prevcids[i];
//    unsigned int loc = prevlocs[i];
//    string fetchkey = stripename + ":" + to_string(cid);
//    cout << "OECWorker::fetchCompute.fetch key = " << fetchkey << ", loc = " << RedisUtil::ip2Str(loc) << endl;
//  }
//  for (auto item:coefs) {
//    int cid = item.first;
//    vector<int> coef = item.second;
//    string writekey = stripename + ":" + to_string(cid);
//    cout << "OECWorker::fetchCompute.compute " << cid << " with coefs ( ";
//    for (int i=0; i<coef.size(); i++) {
//      cout << coef[i] << " ";
//    }
//    cout << "), write as " << writekey << endl;
//  }
//
//  int pktsize = _conf->_pktSize;
//  int slicesize = pktsize/scratio;
//
//  // create thread to read data from disk
//  FSObjInputStream* objstream = new FSObjInputStream(_conf, objname, _underfs);
//  if (!objstream->exist()) {
//    cout << "OECWorker::readWorker." << objname << " does not exist!" << endl;
//    return;
//  }
////  thread readThread = thread([=]{objstream->readObj(slicesize, unitidx);});
//  BlockingQueue<OECDataPacket*>* readQueue = objstream->getQueue();
//
//  // create queue to fetch data from remote
//  BlockingQueue<OECDataPacket*>** fetchQueue = (BlockingQueue<OECDataPacket*>**)calloc(nprevs, sizeof(BlockingQueue<OECDataPacket*>*));
//  for (int i=0; i<nprevs; i++) {
//    if (prevcids[i] == cid) {
//      fetchQueue[i] = readQueue;
//    } else {
//      fetchQueue[i] = new BlockingQueue<OECDataPacket*>();
//    }
//  }
//
//  // create write queue
//  BlockingQueue<OECDataPacket*>** writeQueue = (BlockingQueue<OECDataPacket*>**)calloc(coefs.size(), sizeof(BlockingQueue<OECDataPacket*>*));
//  for (int i=0; i<coefs.size(); i++) {
//    writeQueue[i] = new BlockingQueue<OECDataPacket*>();
//  }
//
//  // create thread to fetch data
//  vector<thread> fetchThreads = vector<thread>(nprevs);
//  for (int i=0; i<nprevs; i++) {
//    if (prevcids[i] == cid) {
//      fetchThreads[i] = thread([=]{objstream->readObj(pktsize);});
//    } else {
//      string keybase = stripename+":"+to_string(prevcids[i]);
//      fetchThreads[i] = thread([=]{fetchWorker(fetchQueue[i], keybase, prevlocs[i], num);});
//    }
//  }
//
//  // create compute thread
//  thread computeThread = thread([=]{computeWorker(fetchQueue, nprevs, num, coefs, computefor, writeQueue, _conf->_pktSize/scratio);});
//
//  // create write thread
//  vector<thread> writeThreads = vector<thread>(computefor.size());
//  for (int i=0; i<computefor.size(); i++) {
//    string keybase = stripename+":"+to_string(computefor[i]);
//    writeThreads[i] = thread([=]{writeWorker(writeQueue[i], keybase, num);});
//  }
//
//  // join
////  readThread.join();
//  for (int i=0; i<nprevs; i++) {
//    fetchThreads[i].join();
//  }
//  computeThread.join(); 
//  for (int i=0; i<computefor.size(); i++) {
//    writeThreads[i].join();
//  }  
//
//  // delete
//  for (int i=0; i<nprevs; i++) {
//    if (prevcids[i] == cid) delete objstream;
//    else delete fetchQueue[i];
//  }
//  free(fetchQueue);
//  for (int i=0; i<computefor.size(); i++) {
//    delete writeQueue[i];
//  }
//  free(writeQueue);
//  cout << "OECWorker::readFetchCompute finishes!" << endl;
//}
//
//void OECWorker::fetchCompute(AGCommand* agcmd) {
//  cout << "OECWorker::fetchCompute" << endl;
//  string stripename = agcmd->_stripeName;
//  int scratio = agcmd->_scratio;
//  int num = agcmd->_num;
//  int nprevs = agcmd->_nprevs;
//  vector<int> prevcids = agcmd->_prevCids;
//  vector<unsigned int> prevlocs = agcmd->_prevLocs;
//  unordered_map<int, vector<int>> coefs = agcmd->_coefs;
//  vector<int> computefor;
//  for (auto item:coefs) {
//    computefor.push_back(item.first);
//  }
//  // xiaolu comment 20180825 start
////  for (int i=0; i<nprevs; i++) {
////    int cid = prevcids[i];
////    unsigned int loc = prevlocs[i];
////    string fetchkey = stripename + ":" + to_string(cid);
////    cout << "OECWorker::fetchCompute.fetch key = " << fetchkey << ", loc = " << RedisUtil::ip2Str(loc) << endl;
////  }
////  for (auto item:coefs) {
////    int cid = item.first;
////    vector<int> coef = item.second;
////    string writekey = stripename + ":" + to_string(cid);
////    cout << "OECWorker::fetchCompute.compute " << cid << " with coefs ( ";
////    for (int i=0; i<coef.size(); i++) {
////      cout << coef[i] << " ";
////    }
////    cout << "), write as " << writekey << endl;
////  }
//  // xiaolu comment 20180825 end
//
//  // create fetch queue
//  BlockingQueue<OECDataPacket*>** fetchQueue = (BlockingQueue<OECDataPacket*>**)calloc(nprevs, sizeof(BlockingQueue<OECDataPacket*>*));
//  for (int i=0; i<nprevs; i++) {
//    fetchQueue[i] = new BlockingQueue<OECDataPacket*>();
//  }
//
//  // create write queue
//  BlockingQueue<OECDataPacket*>** writeQueue = (BlockingQueue<OECDataPacket*>**)calloc(coefs.size(), sizeof(BlockingQueue<OECDataPacket*>*));
//  for (int i=0; i<coefs.size(); i++) {
//    writeQueue[i] = new BlockingQueue<OECDataPacket*>();
//  }
//
//  // create fetch thread
//  vector<thread> fetchThreads = vector<thread>(nprevs);
//  for (int i=0; i<nprevs; i++) {
//    string keybase = stripename+":"+to_string(prevcids[i]);
//    fetchThreads[i] = thread([=]{fetchWorker(fetchQueue[i], keybase, prevlocs[i], num);});
//  }
//
//  // create compute thread
//  thread computeThread = thread([=]{computeWorker(fetchQueue, nprevs, num, coefs, computefor, writeQueue, _conf->_pktSize/scratio);});
//
//  // create write thread
//  vector<thread> writeThreads = vector<thread>(computefor.size());
//  for (int i=0; i<computefor.size(); i++) {
//    string keybase = stripename+":"+to_string(computefor[i]);
//    writeThreads[i] = thread([=]{writeWorker(writeQueue[i], keybase, num);});
//  }
//
//
//  // join
//  for (int i=0; i<nprevs; i++) {
//    fetchThreads[i].join();
//  }
//  computeThread.join(); 
//  for (int i=0; i<computefor.size(); i++) {
//    writeThreads[i].join();
//  }
//
//  // delete
//  for (int i=0; i<nprevs; i++) {
//    delete fetchQueue[i];
//  }
//  free(fetchQueue);
//  for (int i=0; i<computefor.size(); i++) {
//    delete writeQueue[i];
//  }
//  free(writeQueue);
//  cout << "OECWorker::fetchCompute finishes!" << endl;
//}
//
//void OECWorker::persist(AGCommand* agcmd) {
//  cout << "OECWorker::persist" << endl;
//  string stripename = agcmd->_stripeName;
//  int scratio = agcmd->_scratio;
//  int num = agcmd->_num;
//  int nprevs = agcmd->_nprevs;
//  vector<int> prevcids = agcmd->_prevCids;
//  vector<unsigned int> prevlocs = agcmd->_prevLocs;
//  string objname = agcmd->_writeObjName;
//  for (int i=0; i<nprevs; i++) {
//    string keybase = stripename+":"+to_string(prevcids[i]);
//    cout << "OECWorker::persist.fetch "<<keybase<<" from " << RedisUtil::ip2Str(prevlocs[i]) << endl;
//  }
//  cout << "OECWorker::persist.write as " << objname << " with " << num << " pkts"<< endl;
//
//  // create fetch queue
//  BlockingQueue<OECDataPacket*>** fetchQueue = (BlockingQueue<OECDataPacket*>**)calloc(nprevs, sizeof(BlockingQueue<OECDataPacket*>*));
//  for (int i=0; i<nprevs; i++) {
//    fetchQueue[i] = new BlockingQueue<OECDataPacket*>();
//  }
//
//  // create fetch thread
//  vector<thread> fetchThreads = vector<thread>(nprevs);
//  for (int i=0; i<nprevs; i++) {
//    string keybase = stripename+":"+to_string(prevcids[i]);
//    fetchThreads[i] = thread([=]{fetchWorker(fetchQueue[i], keybase, prevlocs[i], num);});
//  }
//
//  // create objstream and writeThread
//  FSObjOutputStream* objstream = new FSObjOutputStream(_conf, objname, _underfs);
//  thread writeThread = thread([=]{objstream->writeObj();});
//
//  int total = num;
//  while(total--) {
////    cout << "OECWorker::persist.left = " << total << endl;
//    for (int i=0; i<nprevs; i++) {
//      OECDataPacket* curpkt = fetchQueue[i]->pop();
//      objstream->enqueue(curpkt);
//    }
//  }
//
//  // we need to add an end pkt to objstrea
//  OECDataPacket* endpkt = new OECDataPacket(0);
//  objstream->enqueue(endpkt);
//
//  // join 
//  for (int i=0; i<nprevs; i++) {
//    fetchThreads[i].join();
//  }
//  writeThread.join();
//
//  // delete
//  for (int i=0; i<nprevs; i++) {
//    delete fetchQueue[i];
//  }
//  free(fetchQueue);
//  if (objstream) delete objstream;
//
//  // write a finish flag to local?
//  // writefinish:objname
//  redisReply* rReply;
//  redisContext* writeCtx = RedisUtil::createContext(_conf->_localIp);
//
//  string wkey = "writefinish:" + objname;
//  int tmpval = htonl(1);
//  rReply = (redisReply*)redisCommand(writeCtx, "rpush %s %b", wkey.c_str(), (char*)&tmpval, sizeof(tmpval));
//  freeReplyObject(rReply);
//  redisFree(writeCtx);
//  cout << "OECWorker::persist finishes!" << endl;
//}
//
//void OECWorker::clientRead(AGCommand* agcmd) {
//  cout << "OECWorker::clientRead" << endl;
//  string filename = agcmd->_filename;
//
//  struct timeval coor1, coor2;
//  gettimeofday(&coor1, NULL);
//  // 0. send request to coordinator to get filemeta
//  CoorCommand* coorCmd = new CoorCommand();
//  coorCmd->buildType3(3, _conf->_localIp, filename);
//  coorCmd->sendTo(_coorCtx);
//
//  delete coorCmd;
// 
//  // 1. wait for metadata
//  int redundancy;
//  int filesizeMB;
//  // for online
//  string ecid;
//  // for offline
//  string poolname;
//  string stripename;
//  
//  string metakey = "filemeta:"+filename;
//  redisReply* metareply;
//  redisContext* metaCtx = RedisUtil::createContext(_conf->_localIp);
//  metareply = (redisReply*)redisCommand(metaCtx, "blpop %s 0", metakey.c_str());
//  char* metastr = metareply->element[1]->str;
//  gettimeofday(&coor2, NULL);
//  cout << "OECWorker::clientRead.get metadata from coordinator: " << RedisUtil::duration(coor1, coor2) << endl;
//
//  // 1.1 redundancy
//  memcpy((char*)&redundancy, metastr, 4); metastr += 4;
//  redundancy = ntohl(redundancy);
//  // 1.2 filesizeMB
//  memcpy((char*)&filesizeMB, metastr, 4); metastr += 4;
//  filesizeMB = ntohl(filesizeMB);
//  cout << "OECWorker::clientRead.redundancy = " << redundancy << ", filesizeMB = " << filesizeMB << endl;
//  if (redundancy == 0) {
//    // online encode
//    // 1.3 ecidlen
//    int tmplen;
//    memcpy((char*)&tmplen, metastr, 4); metastr += 4;
//    tmplen = ntohl(tmplen);
//    // 1.4 ecid
//    char* ecidstr = (char*)calloc(tmplen+1, sizeof(char));
//    memcpy(ecidstr, metastr, tmplen); metastr += tmplen;
//    ecid = string(ecidstr);
//    free(ecidstr);
//    cout << "OECWorker::clientRead.ecid = " << ecid << endl;
//  } else {
//    // offline
//    // 1.3 poolname
//    // lenpoolname
//    int lenpoolname;
//    memcpy((char*)&lenpoolname, metastr, 4); metastr += 4;
//    lenpoolname = ntohl(lenpoolname);
//    // poolname
//    char* poolnamestr = (char*)calloc(lenpoolname+1, sizeof(char));
//    memcpy(poolnamestr, metastr, lenpoolname); metastr += lenpoolname;
//    poolname = string(poolnamestr);
//    free(poolnamestr);
//    cout << "OECWorker::clientRead.poolname = " << poolname << endl;
//    // 1.4 stripename
//    // len stripename
//    int lenstripename;
//    memcpy((char*)&lenstripename, metastr, 4); metastr += 4;
//    lenstripename = ntohl(lenstripename);
//    // stripename
//    char* stripenamestr = (char*)calloc(lenstripename+1, sizeof(char));
//    memcpy(stripenamestr, metastr, lenstripename); metastr += lenstripename;
//    stripename = string(stripenamestr);
//    free(stripenamestr);
//    cout << "OECWOrker::clientRead.stripename = " << stripename << endl;
//  }
//   // 1.last free
//   freeReplyObject(metareply);
//   redisFree(metaCtx);
// 
//   // xiaolu add 20180816 start
//   ECPolicy* ecpolicy = _conf->_ecPolicyMap[ecid];
//   // xiaolu add 20180816 end
// 
//   // 2. return filesizeMB to client
//   struct timeval retcli1, retcli2;
//   gettimeofday(&retcli1, NULL);
//   redisReply* rReply;
//   redisContext* cliCtx = RedisUtil::createContext(_conf->_localIp);
//   string skey = "filesize:"+filename;
//   int tmpval = htonl(filesizeMB);
//   rReply = (redisReply*)redisCommand(cliCtx, "rpush %s %b", skey.c_str(), (char*)&tmpval, sizeof(tmpval));
//   freeReplyObject(rReply);
// 
////   // TODO: return stream num to client
////   // xiaolu add 20180816 start
////   skey = "streamnum:"+filename;
////   int streamnum=ecpolicy->_k;
////   tmpval = htonl(streamnum);
////   rReply = (redisReply*)redisCommand(cliCtx, "rpush %s %b", skey.c_str(), (char*)&tmpval, sizeof(tmpval));
////   freeReplyObject(rReply);
////   // xiaolu add 20180816 end
// 
//   redisFree(cliCtx);
//   gettimeofday(&retcli2, NULL);
//   cout << "OECWorker::clientRead.return filesize to client: " << RedisUtil::duration(retcli1, retcli2) << endl;
// 
//   // TODO: if redundancy == offline enc, we change to another function
//   if (redundancy == 1) return clientReadOffline(filename, filesizeMB, poolname, stripename);
// 
//   // 4. check integrity
// //  ECPolicy* ecpolicy = _conf->_ecPolicyMap[ecid];
//  vector<int> integrity;
//  FSObjInputStream** objstreams = (FSObjInputStream**)calloc(ecpolicy->_n, sizeof(FSObjInputStream*));
//  int streamidx = 0;
//  for (int i=0; i<ecpolicy->_n; i++) {
//    string objname = filename+"_oecobj_"+to_string(i);
//    objstreams[i] = new FSObjInputStream(_conf, objname, _underfs);
//    if (objstreams[i]->exist()) integrity.push_back(1);
//    else integrity.push_back(0);
//  }
//
//  bool needRecovery = false;
//  for (int i=0; i<ecpolicy->_k; i++) {
//    if (integrity[i] == 0) {
//      needRecovery = true;
//
//      // update stripestore
//      CoorCommand* updateCmd = new CoorCommand(); 
//      updateCmd->buildType10(10, _conf->_localIp, filename+"_oecobj_"+to_string(i));
//      updateCmd->sendTo(_conf->_coorIp);
//      break;
//    }
//  }
//
//  // 5. decide to read from which input streams
//  FSObjInputStream** readStreams = NULL;
//  vector<int> idlist;
//  ECBase* ec = NULL;
//  vector<int> avail;
//  vector<int> torec;
//  if (needRecovery) {
//    cout << "OECWorker::clientRead.needRecovery" << endl;
//    ec = ecpolicy->createECClass();
//    // figure out the avail and torec list
//    for (int i=0; i<ecpolicy->_k; i++) {
//      if (integrity[i]) {
//        // this storage unit is available, add corresponding computation unit into avail
//        for (int j=0; j<ec->_cps; j++) avail.push_back(i*ec->_cps+j);
//      } else {
//        // this storage unit is lost, add corresponding computation unit into torec
//        for (int j=0; j<ec->_cps; j++) torec.push_back(i*ec->_cps+j);
//      }
//    }
//    for (int i=ecpolicy->_k; i<ecpolicy->_n; i++) {
//      if (integrity[i]) {
//        for (int j=0; j<ec->_cps; j++) avail.push_back(i*ec->_cps+j);
//      }
//    }
//    cout << "OECWorker::clientRead.needRecovery.avail = ";
//    for (auto item: avail) cout << item << " ";
//    cout << endl;
//    cout << "OECWorker::clientRead.needRecovery.torec = ";
//    for (auto item: torec) cout << item << " ";
//    cout << endl;
//
//    // prepare decode ECDAG
//    ec->decode(avail, torec);
//    ec->dump();
//
//    // traverse the ECDAG nad figure out the idlist corresponding to recovery
//    vector<int> cidlist = ec->getCidList();
//    sort(cidlist.begin(), cidlist.end());
//    cout << "OECWorker::clientRead.cidlist:";
//    for (auto item: cidlist) cout << item << " ";
//    cout << endl;
//
//    for (int i=0; i<cidlist.size(); i++) {
//      int cursid = cidlist[i]/ec->_cps;
//      int listsize = idlist.size();
//      if (listsize>0 && idlist[listsize-1] == cursid) continue;
//      else if (cursid < ec->_n) idlist.push_back(cursid);
//    }
//
//    // Then we only keep such items in avail: (1) item/cps < k; (2) item is in cidlist
//    avail.clear();
//    for (int i=0; i<ec->_k; i++) {
//      if (integrity[i]) {
//        for (int j=0; j<ec->_cps; j++) avail.push_back(i*ec->_cps+j);
//      }
//    }
//    for (auto cid: cidlist) {
//      if (cid/ec->_cps >= ec->_k && cid/ec->_cps < ec->_n) avail.push_back(cid);
//    }
//    sort(avail.begin(), avail.end());
//
//    // then we create corresponding input stream for idlist
//    readStreams = (FSObjInputStream**)calloc(idlist.size(), sizeof(FSObjInputStream*));
//    for (int i=0; i<idlist.size(); i++) {
//      int sid = idlist[i];
//      readStreams[i] = objstreams[sid];
//    }
//    cout << "idlist = ";
//    for (int i=0; i<idlist.size(); i++) cout << idlist[i] << " ";
//    cout << endl;
//
//    cout << "updated avail = ";
//    for (auto item: avail) cout << item << " ";
//    cout << endl;
//  } else {
//    cout << "OECWorker::clientRead.do not needRecovery" << endl;
//    readStreams = (FSObjInputStream**)calloc(ecpolicy->_k, sizeof(FSObjInputStream*));
//    int readidx=0;
//    for (int i=0; i<ecpolicy->_k; i++) {
//      readStreams[readidx++] = objstreams[i];
//      idlist.push_back(i);
//    }
//  }
//
//  // 6. create threads to read data
//  vector<thread> readThreads = vector<thread>(idlist.size());
//  for (int i=0; i<idlist.size(); i++) {
//    readThreads[i] = thread([=]{readStreams[i]->readObj();});
//  }
//
//  // 7. create a queue to cache the original data
//  // TODO: create k queue to cache original data
//  // xiaolu comment 20180815 start
//  BlockingQueue<OECDataPacket*>* writeQueue = new BlockingQueue<OECDataPacket*>();
//  // xiaolu comment 20180815 end
//
//  // xiaolu add 20180815 start
////  BlockingQueue<OECDataPacket*>** clientWriteQueue = (BlockingQueue<OECDataPacket*>**)calloc(ecpolicy->_k, sizeof(BlockingQueue<OECDataPacket*>*));
////  for (int i=0; i<ecpolicy->_k; i++) {
////    clientWriteQueue[i] = new BlockingQueue<OECDataPacket*>();
////  }
//  // xiaolu add 20180815 end
//   
//  // 8. decode worker
//  // TODO: pass data into k queue
//  // xiaolu comment 20180815 start
//  thread decThread([=]{decWorker(readStreams, idlist, needRecovery, writeQueue, ec, avail, torec);});
//  // xiaolu comment 20180815 end
//
//  // xiaolu add 20180815 start
////  thread decThread([=]{decWorker(readStreams, idlist, needRecovery, clientWriteQueue, ec, avail, torec);});
//  // xiaolu add 20180815 end
//
//  // 9. write worker
//  // TODO: create k writeWorkers to write data into local redis
//  int pktnum = 1048576 * filesizeMB / _conf->_pktSize;
//  // xiaolu command 20180815 start
//  thread writeThread([=]{writeWorker(writeQueue, filename);});
//  // xiaolu comment 20180815 end
//
//  // xiaolu add 20180815 start version 1
////  vector<thread> writeThreads(ecpolicy->_k);
////  for (int i=0; i<ecpolicy->_k; i++) {
////    writeThreads[i] = thread([=]{writeWorker(clientWriteQueue[i], filename, i, ecpolicy->_k, pktnum+1);});
////  }
//  // xiaolu add 20180815 end
//  
//  // xiaolu comment 20180815 start
//  // join
//  for (int i=0; i<idlist.size(); i++) {
//    readThreads[i].join();
//  }
//  decThread.join();
//  writeThread.join(); 
//  // xiaolu comment 20180815 end
//
//  // xiaolu add 20180815 start
////  for (int i=0; i<idlist.size(); i++) readThreads[i].join();
////  decThread.join();
////  for (int i=0; i<ecpolicy->_k; i++) writeThreads[i].join();
//  // xiaolu add 20180815 end
//
//  // last. free
//  for (int i=0; i<ecpolicy->_n; i++) {
//    if (objstreams[i]) delete objstreams[i];
//  }
//  if (objstreams) free(objstreams);
//  if (readStreams) free(readStreams);
//  if (ec) delete ec;
//  // xiaolu comment 20180815 start
//  delete writeQueue;
//  // xiaolu comment 20180815 end
//
//  // xiaolu add 20180815 start
////  for (int i=0; i<ecpolicy->_k; i++) delete clientWriteQueue[i];
////  free(clientWriteQueue);
//  // xiaolu add 20180815 end
//}
//
//void OECWorker::offlineWrite(AGCommand* agcmd) {
//  cout << "OECWorker::offlineWrite.filename = " << agcmd->_filename << " ";
//  cout << "mode = " << agcmd->_mode << " ";
//  cout << "pool = " << agcmd->_ecid << " ";
//  cout << "filesizeMB = " << agcmd->_filesizeMB << endl;
//
//  string filename = agcmd->_filename;
//  string ecpool = agcmd->_ecid;
//  string mode = agcmd->_mode;
//  int modeint = 1;
//  int filesizeMB = agcmd->_filesizeMB;
//
//  struct timeval time1, time2;
//  gettimeofday(&time1, NULL);
//
//  // register the filename with ecpool
//  CoorCommand* coorCmd = new CoorCommand();
//  coorCmd->buildType0(0, _conf->_localIp, filename, ecpool, modeint);
//  coorCmd->sendTo(_coorCtx);
//
//  // TODO: wait for response?
//  redisReply* rReply;
//  redisContext* waitCtx = RedisUtil::createContext(_conf->_localIp);
//  string wkey = "updateMeta:" + filename;
//  rReply = (redisReply*)redisCommand(waitCtx, "blpop %s 0", wkey.c_str());
//  freeReplyObject(rReply);
//  redisFree(waitCtx);
//
//  delete coorCmd;
//
//  gettimeofday(&time2, NULL);
//  cout << "OECWorker::offlineWrite.register filename with ecpool.duration : " << RedisUtil::duration(time1, time2)<< endl;
//
//  // create outputstream
//  FSObjOutputStream* objstream = new FSObjOutputStream(_conf, filename, _underfs);
//  BlockingQueue<OECDataPacket*>* readQueue = objstream->getQueue(); 
//
//  // create readThread
////  int pktnum = 1048576/_conf->_pktSize * filesizeMB + 1;
//  int pktnum = 1048576 * filesizeMB / _conf->_pktSize + 1;
//  thread readThread([=]{readWorker(readQueue, filename, pktnum);});
//  thread writeThread([=]{objstream->writeObj();});
//
//  // join
//  readThread.join();
//  writeThread.join();
//
//  // check finish flag in stream
//  if (objstream->getFinish()) {
//    // writefinish:filename
//    redisReply* rReply;
//    waitCtx = RedisUtil::createContext(_conf->_localIp);
//
//    string wkey = "writefinish:" + filename;
//    int tmpval = htonl(1);
//    rReply = (redisReply*)redisCommand(waitCtx, "rpush %s %b", wkey.c_str(), (char*)&tmpval, sizeof(tmpval));
//    freeReplyObject(rReply);
//    redisFree(waitCtx);
//  }
//
//  // TODO: update filesize to coordinator
//  CoorCommand* coorCmd1 = new CoorCommand();
//  coorCmd1->buildType2(2, _conf->_localIp, filename, filesizeMB);
//  coorCmd1->sendTo(_coorCtx);
//  delete coorCmd1;
//
//  delete objstream;
//}
//
//void OECWorker::clientWrite(AGCommand* agcmd) {
//  cout << "oecworker::clientWrite.filename = " << agcmd->_filename << " ";
//  cout << "ecid = " << agcmd->_ecid << " ";
//  cout << "mode =" << agcmd->_mode << " ";
//  cout << "filesizeMB = " << agcmd->_filesizeMB << endl;
//
//  int modeint;
//  if (agcmd->_mode == "online") modeint = 0;
//  else {
//    modeint = 1;
//    return offlineWrite(agcmd);
//  }
//
//  struct timeval time1, time2, time3, time4;
//
//  struct timeval update1, update2;
//  
//  string filename = agcmd->_filename;
//  string ecid = agcmd->_ecid;
//  string mode = agcmd->_mode;
//  int filesizeMB = agcmd->_filesizeMB;
//  gettimeofday(&update1, NULL);
//  // TODO: register the filename with ecid
//  CoorCommand* coorCmd = new CoorCommand();
//  coorCmd->buildType0(0, _conf->_localIp, filename, ecid, modeint);
//  coorCmd->sendTo(_coorCtx);
//
//  // wait for response?
//  redisReply* rReply;
//  redisContext* waitCtx = RedisUtil::createContext(_conf->_localIp);
//
//  string wkey = "updateMeta:" + filename;
//  rReply = (redisReply*)redisCommand(waitCtx, "blpop %s 0", wkey.c_str());
//  freeReplyObject(rReply);
//  redisFree(waitCtx);
//
//  gettimeofday(&update2, NULL);
//  cout << "OECWorker::clientWrite.update metadata time = " <<  RedisUtil::duration(update1, update2) << endl;
//
//  delete coorCmd;
//
//  // 0. create ecpolicy according to ecid
//  ECPolicy* ecpolicy = _conf->_ecPolicyMap[ecid];
//
//  int streamNum;
//  if (mode == "online") streamNum = ecpolicy->_n;
//  else streamNum = ecpolicy->_k;
//
//  cout << "OECWorker::clientWrite.streamNum = " << streamNum << endl;
//
//  // 0. create corresponding output stream
//  gettimeofday(&time1, NULL);
//  FSObjOutputStream** objstreams = (FSObjOutputStream**)calloc(streamNum, sizeof(FSObjOutputStream*));
//  for (int i=0; i<streamNum; i++) {
//    string objname = filename+"_oecobj_"+to_string(i);
//    objstreams[i] = new FSObjOutputStream(_conf, objname, _underfs);
//  }
//  cout << "OECWorker::clientWrite.all the output stream is acknowledged. start to deal with client request for " << filename << endl;
//  gettimeofday(&time2, NULL);
//  cout << "OECWorker::clientWrite.create all FSObjOutputStream.time = " << RedisUtil::duration(time1, time2) << endl;
//
//  // 2. start to read and write
//  // BlockingQueue to cache OECDataPacket?
//  // TODO: create k readQueue
//  // xiaolu comment 20180814 start
////  BlockingQueue<OECDataPacket*>* readQueue = new BlockingQueue<OECDataPacket*>();
//  // xiaolu comment 20180814 end
//
//  // xiaolu add 20180814 start
//  BlockingQueue<OECDataPacket*>** fetchQueue = (BlockingQueue<OECDataPacket*>**)calloc(ecpolicy->_k, sizeof(BlockingQueue<OECDataPacket*>*));
//  for (int i=0; i<ecpolicy->_k; i++) {
//    fetchQueue[i] = new BlockingQueue<OECDataPacket*>();
//  }
//  // xiaolu add 20180814 end
//
//  // create readThread read data from redis and put data into readQueue
//  int pktnum = 1048576 * filesizeMB / _conf->_pktSize + 1;
//
//  // TODO: create k readWorker to fetch pkt
//  // xiaolu comment 20180814 start
////  thread readThread([=]{readWorker(readQueue, filename, pktnum);});
//  // xiaolu comment 20180814 end
//
//  // xiaolu add 20180814 start
//  vector<thread> readClientThreads = vector<thread>(ecpolicy->_k);
//  for (int i=0; i<ecpolicy->_k; i++) {
//    readClientThreads[i] = thread([=]{readWorker(fetchQueue[i], filename, i, ecpolicy->_k, pktnum);});
//  }
//  // xiaolu add 20180814 end
//
//  vector<thread> objthreads = vector<thread>(streamNum);
//  for (int i=0; i<streamNum; i++) {  
//    objthreads[i] = thread([=]{objstreams[i]->writeObj();});
//  }
//
//  // create encWorker
//  // TODO: update encode worker
//  // xiaolu comment 20180814 start
////  thread encThread([=]{encWorker(readQueue, objstreams, ecpolicy, mode);});
//  // xiaolu comment 20180814 end
//
//  // xiaolu add 20180814 start
//  thread encThread([=]{encWorker(fetchQueue, objstreams, ecpolicy, mode, pktnum/ecpolicy->_k);});
//  // xiaolu add 20180814 end
//
//  struct timeval join1, join2;
//  gettimeofday(&join1, NULL);
//  // join
//  // xiaolu comment 20180814 start
////  readThread.join();
////  encThread.join();
////  for (int i=0; i<streamNum; i++) {
////    objthreads[i].join();
////  }
//  // xiaolu comment 20180814 end
//
//  // xiaolu add 20180814 start
//  for (int i=0; i<ecpolicy->_k; i++) readClientThreads[i].join();
//  encThread.join();
//  for (int i=0; i<streamNum; i++) objthreads[i].join();
//  // xiaolu add 20180814 end
//
//  gettimeofday(&join2, NULL);
//  cout << "OECWorker::clientWrite.join time = " << RedisUtil::duration(join1, join2) << endl;
//
//  // check the finish flag in streams and then return finish flag for file
//  bool finish = true;
//  for (int i=0; i<streamNum; i++) {
//    if (!objstreams[i]->getFinish()) {
//      finish = false;
//      break;
//    }
//  }
//
//  struct timeval finish1, finish2;
//  gettimeofday(&finish1, NULL);
//  int filesize=0;
//  if (finish) {
//    // writefinish:filename
//    redisReply* rReply;
//    redisContext* waitCtx = RedisUtil::createContext(_conf->_localIp);
//    string wkey = "writefinish:" + filename;
//    int tmpval = htonl(1);
//    rReply = (redisReply*)redisCommand(waitCtx, "rpush %s %b", wkey.c_str(), (char*)&tmpval, sizeof(tmpval));
//    freeReplyObject(rReply);
//    redisFree(waitCtx);
//  }
//  gettimeofday(&finish2, NULL);
//  cout << "OECWorker::clientWrite.writeback finish.time = " << RedisUtil::duration(finish1, finish2) << endl;
//  cout << "OECWorker::clientWrite.filesizeMB = " << filesizeMB << endl;
//
//  // TODO: update filesize to the coordinator
//  CoorCommand* coorCmd1 = new CoorCommand();
//  coorCmd1->buildType2(2, _conf->_localIp, filename, filesizeMB);
//  coorCmd1->sendTo(_coorCtx);
//  delete coorCmd1;
//
//  // free spaces
//  // xiaolu comment 20180814 start
////  delete readQueue;
////  for (int i=0; i<streamNum; i++) delete objstreams[i];
////  free(objstreams);
//  // xiaolu comment 20180814 end
//  for (int i=0; i<ecpolicy->_k; i++) delete fetchQueue[i];
//  free(fetchQueue);
//  for (int i=0; i<streamNum; i++) delete objstreams[i];
//  free(objstreams);
//}
//
//void OECWorker::readWorker(BlockingQueue<OECDataPacket*>* readQueue,
//                           string keybase,
//                           int startid,
//                           int step,
//                           int max) {
//  struct timeval time1, time2, time3;
//  redisContext* readCtx = RedisUtil::createContext(_conf->_localIp);
//  gettimeofday(&time1, NULL);
//  int count=0;
//  for (int i=startid; i<max; i=i+step) {
//    string key = keybase + ":" + to_string(i);
//    redisAppendCommand(readCtx, "blpop %s 1", key.c_str());
//    count++;
//  }
//  redisReply* rReply;
//  for (int i=0; i<count; i++) {
//    redisGetReply(readCtx, (void**)&rReply);
//    char* content = rReply->element[1]->str;
//    OECDataPacket* pkt = new OECDataPacket(content);
//    int curDataLen = pkt->_dataLen;
//    readQueue->push(pkt);
//    freeReplyObject(rReply);
//  }
//  gettimeofday(&time2, NULL);
//  redisFree(readCtx);
//  cout << "OECWorker::readWorker.readWorker.duration = " << RedisUtil::duration(time1, time2) << endl;
//}
//
//void OECWorker::readWorker(BlockingQueue<OECDataPacket*>* readQueue,
//                           string keybase,
//                           int num) {
//  struct timeval time1, time2, time3;
//  redisContext* readCtx = RedisUtil::createContext(_conf->_localIp);
//  gettimeofday(&time1, NULL);
//  for (int i=0; i<num; i++) {
//    string key = keybase + ":" + to_string(i);
//    redisAppendCommand(readCtx, "blpop %s 1", key.c_str());
//  }
//  redisReply* rReply;
//  for (int i=0; i<num; i++) {
//    redisGetReply(readCtx, (void**)&rReply);
//    char* content = rReply->element[1]->str;
//    OECDataPacket* pkt = new OECDataPacket(content);
//    int curDataLen = pkt->_dataLen;
//    readQueue->push(pkt);
//    freeReplyObject(rReply);
//  }
//  gettimeofday(&time2, NULL);
//  redisFree(readCtx);
//  cout << "OECWorker::readWorker.readWorker.duration = " << RedisUtil::duration(time1, time2) << endl;
//}
//
//void OECWorker::encWorker(BlockingQueue<OECDataPacket*>** fetchQueue,
//                          FSObjOutputStream** objstreams,
//                          ECPolicy* ecpolicy,
//                          string mode,
//                          int stripenum) {
//  ECBase* ec = ecpolicy->createECClass();
//
//  // encode data and code
//  vector<int> data;
//  vector<int> code;
//  for (int i=0; i<ec->_k * ec->_cps; i++) data.push_back(i);
//  for (int i=ec->_k * ec->_cps; i<ec->_n*ec->_cps; i++) code.push_back(i);
//
//  // encode tree
//  ec->encode(data, code);
//  ec->dump();
//
//  struct timeval time1, time2, time3;
//  struct timeval ce1, ce2;
//  double cetime=0;
//
//  int streamNum = ec->_n;
//  OECDataPacket** curStripe = (OECDataPacket**)calloc(streamNum, sizeof(OECDataPacket*));
//  cout << "OECWorker::encWorker.streamNum = " << streamNum << endl;
//
//  gettimeofday(&time1, NULL)  ;
//
//  for (int stripeid=0; stripeid<stripenum; stripeid++) {
//    for (int pktidx=0; pktidx < ec->_k; pktidx++) {
//      OECDataPacket* curPkt = fetchQueue[pktidx]->pop();
//      if (curPkt->_dataLen == 0) {
//        // this can only happen 
//        cout << "OECWorker::encWorker.finish fetch datapackets" << endl;
//        break;
//      }
//      curStripe[pktidx] = curPkt;
//    }
//    // now we have k pkt in a stripe
//    int pktsize = _conf->_pktSize;
//    int splitsize = pktsize / ec->_cps;
//    // prepare pkt for parity pkt
//    for (int i=0; i<(ec->_n-ec->_k); i++) {
//      OECDataPacket* paritypkt = new OECDataPacket(pktsize);
//      curStripe[ec->_k+i] = paritypkt;
//    }
//    // prepare data structures
//    unordered_map<int, char*> bufMap;
//    unordered_map<int, bool> cenforceMap;
//    for (int i=0; i<ec->_n; i++) {
//      for (int j=0; j<ec->_cps; j++) {
//        int ecidx = i*ec->_cps + j;
//        char* bufaddr = curStripe[i]->_data+j*splitsize;
//        bufMap.insert(make_pair(ecidx, bufaddr));
//      }
//    }
//    // centralized enforcement
//    gettimeofday(&ce1, NULL);
//    ec->CEnforce(bufMap, cenforceMap, splitsize);
//    gettimeofday(&ce2, NULL);
//    cetime += RedisUtil::duration(ce1, ce2);
//    // now we put the stripe of pkts to corresponding writeQueue
//    for (int i=0; i<streamNum; i++) {
//      objstreams[i]->enqueue(curStripe[i]);
//      curStripe[i] = NULL;
//    }
//    unordered_map<int, char*>::iterator it = bufMap.begin();
//    while (it != bufMap.end()) {
//      int sidx = it->first/ec->_cps;
//      if (sidx < streamNum) {
//        curStripe[sidx] = NULL;
//      } else {
//        if (it->second) free(it->second);
//      }
//      it++;
//    }
//  }
//
//  gettimeofday(&time2, NULL);
//  cout << "OECWorker::encWorker.cenforce time = " << cetime << endl;
//  cout << "OECWorker::encWorker.time = " << RedisUtil::duration(time1, time2) << endl;
//
//  // finally, add endpacket for each stream?
//  for (int i=0; i<streamNum; i++) {
//    OECDataPacket* endpkt = new OECDataPacket(0);
//    objstreams[i]->enqueue(endpkt);
//  }
//
//  // free
//  free(curStripe);
//  delete ec;
//}
//
//void OECWorker::encWorker(BlockingQueue<OECDataPacket*>* readQueue,
//                          FSObjOutputStream** objstreams,
//                          ECPolicy* ecpolicy,
//                          string mode) {
//  // 0. create erasure code
//  ECBase* ec;
//  if (mode == "online") {
//    ec = ecpolicy->createECClass();
//
//    // encode data and code
//    vector<int> data;
//    vector<int> code;
//    for (int i=0; i<ec->_k * ec->_cps; i++) data.push_back(i);
//    for (int i=ec->_k * ec->_cps; i<ec->_n*ec->_cps; i++) code.push_back(i);
//
//    // encode tree
//    ec->encode(data, code);
//    ec->dump();
//  }
//
//  // read datapacket one by one in readQueue
//  // divide them into k streams
//  // if mode=="online", create parity packet and put to corresponding stream
//  struct timeval time1, time2, time3;
//  int streamNum;
//  if (mode=="online") streamNum = ec->_n;
//  else streamNum = ec->_k;
//  OECDataPacket** curStripe = (OECDataPacket**)calloc(streamNum, sizeof(OECDataPacket*));
//  cout << "OECWorker::encWorker.streamNum = " << streamNum << endl;
//
//  gettimeofday(&time1, NULL)  ;
//
//  int streamIdx = 0; // this is the next streamidx which we should put packet to
//
//  while (true) {
////    cout <<"OECWorker::encWorker.streamIdx = " << streamIdx << endl;
//    // 0. get a OECDataPacket from readQueue
//    OECDataPacket* curPkt = readQueue->pop();
//    if (curPkt->_dataLen == 0) {
//      cout << "OECWorker::encWorker.finish fetch datapackets" << endl;
//      break;
//    }
//    // 1. then this is a data packet which hold data
//    // 2. if this is offline encoding, directly push to corresponding stream
//    if (mode=="offline") objstreams[streamIdx]->enqueue(curPkt);
//    // 3. else this is online
//    else {
//      // 3.1 first add to curStripe
//      curStripe[streamIdx] = curPkt;
////      cout << "OECWorker::encWorker.add to curStripe["<<streamIdx<<"]" << endl;
//      // 3.2 check whether curStripe is full
//      // not full then get next packet
//      // if full then perform encode
//      // now we simulate the encode process
//      if (streamIdx == (ec->_k-1)) {
//        int pktsize = _conf->_pktSize;
//        int splitsize = pktsize / ec->_cps;
//        for (int i=1; i<=(ec->_n-ec->_k); i++) {
//          OECDataPacket* paritypkt = new OECDataPacket(pktsize);
//          curStripe[streamIdx+i] = paritypkt;
//        }
//
//        // prepare data structures
//        unordered_map<int, char*> bufMap;
//        unordered_map<int, bool> cenforceMap;
//        for (int i=0; i<streamNum; i++) {
//          for (int j=0; j<ec->_cps; j++) {
//            int ecidx = i*ec->_cps + j;
//            char* bufaddr = curStripe[i]->_data+j*splitsize;
//            bufMap.insert(make_pair(ecidx, bufaddr));
//          }
//        }
//
//        // centralized enforcement
//        ec->CEnforce(bufMap, cenforceMap, splitsize);
//
//        // now we put the stripe of pkts to corresponding writeQueue
//        for (int i=0; i<streamNum; i++) {
//          objstreams[i]->enqueue(curStripe[i]);
//          curStripe[i] = NULL;
//        }
//        unordered_map<int, char*>::iterator it = bufMap.begin();
//        while (it != bufMap.end()) {
//          int sidx = it->first/ec->_cps;
//          if (sidx < streamNum) {
//            curStripe[sidx] = NULL;
//          } else {
//            if (it->second) free(it->second);
//          }
//          it++;
//        }
//      }
//    }
//
//    // update next streamidx
//    streamIdx = (streamIdx + 1)%ec->_k;
//  }
//  gettimeofday(&time2, NULL);
//  cout << "OECWorker::encWorker.time = " << RedisUtil::duration(time1, time2) << endl;
//
//  // TODO:check whether there is un-full stripe at last
//  // if there is un-full stripe, then add some zero-packets to create parity data
//  // and add to corresponding streams. zero-packets does not send?
//
//  // finally, add endpacket for each stream?
//  for (int i=0; i<streamNum; i++) {
//    OECDataPacket* endpkt = new OECDataPacket(0);
//    objstreams[i]->enqueue(endpkt);
//  }
//
//  // free
//  free(curStripe);
//  delete ec;
//}
//
//void OECWorker::writeWorker(BlockingQueue<OECDataPacket*>* writeQueue,
//                            string keybase,
//                            int startid,
//                            int step,
//                            int max) {
//  redisContext* writeCtx = RedisUtil::createContext(_conf->_localIp);
//
//  struct timeval time1, time2;
//  gettimeofday(&time1, NULL);
//
//  int count = 0;
//  for (int pktidx=startid; pktidx<max; pktidx=pktidx+step) {
//    OECDataPacket* curPkt = writeQueue->pop();
//    int len = curPkt->_dataLen;
//    string key = keybase + ":" + to_string(pktidx);
//    redisAppendCommand(writeCtx, "RPUSH %s %b", key.c_str(), curPkt->_raw, 4+curPkt->_dataLen);
//    delete curPkt;
//    count++;
//  }
//  redisReply* rReply;
//  for (int i=0; i<count; i++) {
//    redisGetReply(writeCtx, (void**)&rReply);
//    freeReplyObject(rReply);
//  }
//  
//  gettimeofday(&time2, NULL);
//  cout << "OECWorker::writeWorker.duration: " << RedisUtil::duration(time1, time2) << " for " << keybase << endl;
//
//  redisFree(writeCtx);
//
//  cout << "OECWorker::writeWorker.writereadis.duration: " << RedisUtil::duration(time1, time2) << ", ";
//}
//
//void OECWorker::writeWorker2(BlockingQueue<OECDataPacket*>* writeQueue, string keybase) {
//  int pktid=0;
//  redisContext* writeCtx = RedisUtil::createContext(_conf->_localIp);
//
//  struct timeval time1, time2, time3;
//  gettimeofday(&time1, NULL);
//
//  // xiaolu comment 20180816 start normal version
//  while (true) {
//    OECDataPacket* curPkt = writeQueue->pop();
//    int len = curPkt->_dataLen;
//    string key = keybase + ":" + to_string(pktid++);
//    redisReply* rReply = (redisReply*)redisCommand(writeCtx, "RPUSH %s %b", key.c_str(), curPkt->_raw, 4+curPkt->_dataLen);
//
//    // delete curPkt?
//    delete curPkt;
//
//    freeReplyObject(rReply);
//    if (len == 0) break;
//  }
//  // xiaolu comment 20180816 end
//}
//
//void OECWorker::writeWorker(BlockingQueue<OECDataPacket*>* writeQueue, string keybase) {
//  int pktid=0;
//  redisContext* writeCtx = RedisUtil::createContext(_conf->_localIp);
//
//  struct timeval time1, time2, time3;
//  gettimeofday(&time1, NULL);
//
//  // xiaolu add 20180816 start pipeline version
//  int getid=0;
//  redisReply* rReply;
//  while (true) {
//    OECDataPacket* curPkt = writeQueue->pop();
//    int len = curPkt->_dataLen;
//    string key = keybase + ":" + to_string(pktid++);
//    redisAppendCommand(writeCtx, "RPUSH %s %b", key.c_str(), curPkt->_raw, 4+curPkt->_dataLen);
//    delete curPkt;
//    if (len == 0) break;
//    if (pktid > 16) {
//      redisGetReply(writeCtx, (void**)&rReply);
//      freeReplyObject(rReply);
//      getid++;
//    }
//  }
//  gettimeofday(&time2, NULL);
//  for (int i=getid; i<pktid; i++) {
//    redisGetReply(writeCtx, (void**)&rReply);
//    freeReplyObject(rReply);
//  }
//  // xiaolu add 20180816 end
//
////  // xiaolu comment 20180816 start normal version
////  while (true) {
////    OECDataPacket* curPkt = writeQueue->pop();
////    int len = curPkt->_dataLen;
////    string key = keybase + ":" + to_string(pktid++);
////    redisReply* rReply = (redisReply*)redisCommand(writeCtx, "RPUSH %s %b", key.c_str(), curPkt->_raw, 4+curPkt->_dataLen);
////
////    // delete curPkt?
////    delete curPkt;
////
////    freeReplyObject(rReply);
////    if (len == 0) break;
////  }
////  // xiaolu comment 20180816 end
//
//  gettimeofday(&time3, NULL);
//  cout << "OECWorker::writeWorker.writereadis.duration: " << RedisUtil::duration(time1, time2) << ", ";
//  cout << "get all reply: " << RedisUtil::duration(time2, time3) << ",  ";
//  cout << "writeWorker.duration: " << RedisUtil::duration(time1, time3) << endl;
////  cout << "OECWorker::writeWorker.duration: " << RedisUtil::duration(time1, time2) << " for " << keybase << endl;
//
//  redisFree(writeCtx);
//}
//
//
//void OECWorker::decWorker(FSObjInputStream** readStreams,
//                          vector<int> idlist,
//                          bool needRecovery,
//                          BlockingQueue<OECDataPacket*>* writeQueue,
//                          //BlockingQueue<OECDataPacket*>** clientWriteQueue,
//                          ECBase* ec,
//                          vector<int> avail,
//                          vector<int> torec) {
//  struct timeval time1, time2;
//  gettimeofday(&time1, NULL);
//  if (!needRecovery) {
//    // read from readStreams one by one
//    while (true) {
//      for (int i=0; i<idlist.size(); i++) {
//        if (readStreams[i]->hasNext()) {
//          OECDataPacket* curpkt = readStreams[i]->dequeue();
////          cout << "OECWorker::decWorker.read from " << i << ", datalen = " << curpkt->_dataLen << endl;
//          
//          // xiaolu comment 20180815 start
//          if (curpkt->_dataLen) writeQueue->push(curpkt);
//          // xiaolu comment 20180815 end
//
//          // xiaolu start 20180815 start
////          if (curpkt->_dataLen) clientWriteQueue[i]->push(curpkt);
//          // xiaolu start 20180815 end
//        }
//      }
//
//      // check whether to do another round
//      bool goon=false;
//      for (int i=0; i<idlist.size(); i++) {
//        if (readStreams[i]->hasNext()) { goon = true; break;}
//      }
//      if (!goon) break;
////      cout << "OECWorker::decWorker.hasNext = " << goon << endl;
//    }
//  } else {
//    // need recovery
//    OECDataPacket** curStripe = (OECDataPacket**)calloc(ec->_n, sizeof(OECDataPacket*));
//    for (int i=0; i<ec->_n; i++) curStripe[i] = NULL;
//    int splitsize = _conf->_pktSize/ec->_cps;
//    unordered_map<int, char*> bufMap;
//    unordered_map<int, bool> cenforceMap;
//    while (true) {
//      // read from input streams
//      for (int i=0; i<idlist.size(); i++) {
//        int sid = idlist[i];
//        OECDataPacket* curpkt = readStreams[i]->dequeue();
//        curStripe[sid] = curpkt;
////        cout << "OECWorker::decWorker.curStripe[" << sid << "] = readStreams " << i << endl;
//      }
//      // first add avail into bufMap
//      for (int i=0; i<avail.size(); i++) {
//        int cidx = avail[i];
//        int sidx = cidx/ec->_cps;
//        int idps = cidx - sidx * ec->_cps;
//        OECDataPacket* curpkt = curStripe[sidx];
////        cout << "OECWorker::decWorker.avail.cidx = " << cidx << ", sidx = " << sidx << endl;
//        bufMap.insert(make_pair(cidx, curpkt->_data+(idps * splitsize)));
//      }
//      // then create buf for torec
//      for (int i=0; i<torec.size(); i++) {
//        int cidx = torec[i];
//        int sidx = cidx/ec->_cps;
//        int idps = cidx - sidx*ec->_cps;
////        cout << "OECWorker::decWorker.torec.cidx = " << cidx << ", sidx = " << sidx << endl;
//        OECDataPacket* curpkt;
//        if (curStripe[sidx] == NULL) {
//          curpkt = new OECDataPacket(_conf->_pktSize);
//          curStripe[sidx] = curpkt;
//        } else {
//          curpkt = curStripe[sidx];
//        }
//        bufMap.insert(make_pair(cidx, curpkt->_data+(idps * splitsize)));
//      }
//   
//      // perform the decode operation to get the original data
//      ec->CEnforce(bufMap, cenforceMap, splitsize);
//
//      // now get out the first k pkt in curstripe
//      for (int i=0; i<ec->_k; i++) {
//        // xiaolu comment 20180815 start
//        writeQueue->push(curStripe[i]);
//        // xiaolu commend 20180815 end
//
//        // xiaolu add 20180815 start
////        clientWriteQueue[i]->push(curStripe[i]);
//        // xiaolu add 20180815 end
//
//        curStripe[i] = NULL;
//      }
//
//      // free
//      // free additional data buffers in bufMap
//      for (auto item: bufMap) {
//        int cidx = item.first;
//        int sidx = cidx/ec->_cps;
//        if (sidx >= ec->_n) {
//          if (item.second) {
//            free(item.second);
//            item.second = NULL;
//          }
//        }
//      }
//      for (int i=ec->_k; i<ec->_n; i++) {
//        if (curStripe[i]) {
//          delete curStripe[i];
//          curStripe[i] = NULL;
//        }
//      }
//      bufMap.clear();
//      cenforceMap.clear();
//
//
//      // check whether there is another round?
//      bool goon=false;
//      for (int i=0; i<idlist.size(); i++) {
//        if (readStreams[i]->hasNext()) { goon = true; break;}
//      }
//
//      if (!goon) break;
//    }
//  }
//
//  // endpkt
//  OECDataPacket* endpkt = new OECDataPacket(0);
//  // xiaolu command 20180816 start
//  writeQueue->push(endpkt);
//  // xiaolu comment 20180816 end
//
//  // xiaolu add 20180816 start
////  clientWriteQueue[0]->push(endpkt);
//  // xiaolu add 20180816 end
//
//  gettimeofday(&time2, NULL);
//  cout << "OECWorker::decWorker.duration: " << RedisUtil::duration(time1, time2) << endl;
//}
//
//void OECWorker::readWorker(BlockingQueue<OECDataPacket*>* readQueue,
//                    string objname,
//                    int num,
//                    int pktsize,
//                    int slicesize,
//                    int unitidx) {
//  // TODO: get the file handler for the file objname
//  cout << "OECWorker::readWorker.objname: " << objname << ", num: " << num << ", pktsize: " << pktsize << ", slicesize: " << slicesize << ", unitidx: " << unitidx << endl;
//  FSObjInputStream* objstream = new FSObjInputStream(_conf, objname, _underfs);
//  if (!objstream->exist()) {
//    cout << "OECWorker::readWorker." << objname << " does not exist!" << endl;
//    return;
//  }
//  thread readThread = thread([=]{objstream->readObj(slicesize, unitidx);});
//
//  struct timeval time1, time2;
//  gettimeofday(&time1, NULL);
//
//  int curnum = 0;
//  while(curnum < num) {
//    OECDataPacket* curpkt = objstream->dequeue();
//    readQueue->push(curpkt);
//    curnum++;
//  }
//  gettimeofday(&time2, NULL);
//  cout << "OECWorker::readWorker.duration = " << RedisUtil::duration(time1, time2) << " for " << objname << endl;
//
//  // join read thread
//  readThread.join();
//  // delete objstream
//  delete objstream;
//}
//
//void OECWorker::testWriteWorker(BlockingQueue<OECDataPacket*>* writeQueue,
//                            string keybase,
//                            int num) {
//  
//  redisReply* rReply;
//  redisContext* writeCtx = RedisUtil::createContext("127.0.0.1");
//  
//  struct timeval time1, time2;
//  gettimeofday(&time1, NULL);
//
//  // implementation 2
//  for (int i=0; i<num; i++) {
//    string key = keybase+":"+to_string(i);
//    OECDataPacket* curpkt = writeQueue->pop();
//    char* raw = curpkt->_raw;
//    int rawlen = curpkt->_dataLen + 4;
//    redisAppendCommand(writeCtx, "RPUSH %s %b", key.c_str(), raw, rawlen);
//    delete curpkt;
//  }
//  for (int i=0; i<num; i++) {
//    redisGetReply(writeCtx, (void**)&rReply);
//    freeReplyObject(rReply);
//  }
//
//  gettimeofday(&time2, NULL);
//  cout << "OECWorker::testWriteWorker.duration: " << RedisUtil::duration(time1, time2) << " for " << keybase << endl;
//  redisFree(writeCtx);
//}
//
//void OECWorker::writeWorker(BlockingQueue<OECDataPacket*>* writeQueue,
//                            string keybase,
//                            int num) {
//  redisReply* rReply;
//  redisContext* writeCtx = RedisUtil::createContext("127.0.0.1");
//  
//  struct timeval time1, time2;
//  gettimeofday(&time1, NULL);
//
////  // implementation 1
////  for (int i=0; i<num; i++) {
////    string key = keybase+":"+to_string(i);
////    OECDataPacket* curpkt = writeQueue->pop();
////    char* raw = curpkt->_raw;
////    int rawlen = curpkt->_dataLen + 4;
////    rReply = (redisReply*)redisCommand(writeCtx, "RPUSH %s %b", key.c_str(), raw, rawlen); 
////    freeReplyObject(rReply);
////    delete curpkt;
////  }
//
//  // implementation 2
////  // xiaolu comment 20180822 start
////  for (int i=0; i<num; i++) {
////    string key = keybase+":"+to_string(i);
////    OECDataPacket* curpkt = writeQueue->pop();
////    char* raw = curpkt->_raw;
////    int rawlen = curpkt->_dataLen + 4;
////    redisAppendCommand(writeCtx, "RPUSH %s %b", key.c_str(), raw, rawlen);
////    delete curpkt;
////  }
////  for (int i=0; i<num; i++) {
////    redisGetReply(writeCtx, (void**)&rReply);
////    freeReplyObject(rReply);
////  }
////  // xiaolu comment 20180822 end
//
//  // implementation 3
//  // xiaolu add 20180822 start
//  int replyid=0;
//  for (int i=0; i<num; i++) {
//    string key = keybase+":"+to_string(i);
//    OECDataPacket* curpkt = writeQueue->pop();
//    char* raw = curpkt->_raw;
//    int rawlen = curpkt->_dataLen + 4;
//    redisAppendCommand(writeCtx, "RPUSH %s %b", key.c_str(), raw, rawlen);
//    delete curpkt;
//    if (i>1) {
//      redisGetReply(writeCtx, (void**)&rReply);
//      freeReplyObject(rReply);
//      replyid++;
//    }
//  }
//  for (int i=replyid; i<num; i++) {
//    redisGetReply(writeCtx, (void**)&rReply);
//    freeReplyObject(rReply);
//  }
//  // xiaolu start 20180822 end
//
//  gettimeofday(&time2, NULL);
//  cout << "OECWorker::writeWorker.duration: " << RedisUtil::duration(time1, time2) << " for " << keybase << endl;
//  redisFree(writeCtx);
//}
//
//void OECWorker::fetchWorker(BlockingQueue<OECDataPacket*>* fetchQueue,
//                            string keybase,
//                            unsigned int loc,
//                            int num) {
//  redisReply* rReply;
//  redisContext* fetchCtx = RedisUtil::createContext(loc);
//
//  struct timeval time1, time2;
//  gettimeofday(&time1, NULL);
//
//  int replyid=0;
//  // xiaolu comment 20180822 start
//  for (int i=0; i<num; i++) {
//    string key = keybase+":"+to_string(i);
//    redisAppendCommand(fetchCtx, "blpop %s 0", key.c_str());
//  }
//
//  struct timeval t1, t2;
//  double t;
//  for (int i=replyid; i<num; i++) {
//    string key = keybase+":"+to_string(i);
////    cout << "OECWorker::fetchWorker.fetch " << key << endl;
//    gettimeofday(&t1, NULL);
//    redisGetReply(fetchCtx, (void**)&rReply);
//    gettimeofday(&t2, NULL);
//    if (i == 0) cout << "OECWorker::fetchWorker.fetch first t = " << RedisUtil::duration(t1, t2) << endl;
//    char* content = rReply->element[1]->str;
//    OECDataPacket* pkt = new OECDataPacket(content);
//    int curDataLen = pkt->_dataLen;
//    fetchQueue->push(pkt);
//    freeReplyObject(rReply);
//  }
//  // xiaolu comment 20180822 end
//
//  // xiaolu add 20180822 start
////  for (int i=0; i<num; i++) {
////    string key = keybase+":"+to_string(i);
////    rReply = (redisReply*)redisCommand(fetchCtx, "blpop %s 0", key.c_str()); 
////    char* content = rReply->element[1]->str;
////    OECDataPacket* pkt = new OECDataPacket(content);
////    int curDataLen = pkt->_dataLen;
////    fetchQueue->push(pkt);
////    freeReplyObject(rReply);
////  }
//  // xiaolu add 20180822 end
//
//  gettimeofday(&time2, NULL);
//  cout << "OECWorker::fetchWorker.duration: " << RedisUtil::duration(time1, time2) << " for " << keybase << endl;
//  redisFree(fetchCtx);
//}
//
//void OECWorker::computeWorker(BlockingQueue<OECDataPacket*>** fetchQueue,
//                       int nprev,
//                       int num,
//                       unordered_map<int, vector<int>> coefs,
//                       vector<int> cfor,
//                       BlockingQueue<OECDataPacket*>** writeQueue,
//                       int slicesize) {
//  // prepare coding matrix
//  int row = cfor.size();
//  int col = nprev;
//  int* matrix = (int*)calloc(row*col, sizeof(int));
//  for (int i=0; i<row; i++) {
//    int cid = cfor[i];
//    vector<int> coef = coefs[cid];
//    for (int j=0; j<col; j++) {
//      matrix[i*col+j] = coef[j];
//    }
//  }
//  cout << "OECWorker::computeWorker.num: " << num << ", row: " << row << ", col: " << col << endl;
//  cout << "-------------------"<< endl;
//  for (int i=0; i<row; i++) {
//    for (int j=0; j<col; j++) {
//      cout << matrix[i*col+j] << " ";
//    }
//    cout << endl;
//  }
//  cout << "-------------------"<< endl;
//
//  OECDataPacket** curstripe = (OECDataPacket**)calloc(row+col, sizeof(OECDataPacket*));
//  char** data = (char**)calloc(col, sizeof(char*));
//  char** code = (char**)calloc(row, sizeof(char*));
//  while(num--) {
//    // prepare data
//    for (int i=0; i<col; i++) {
//      OECDataPacket* curpkt = fetchQueue[i]->pop();
//      curstripe[i] = curpkt;
//      data[i] = curpkt->_data;
//    }
//    for (int i=0; i<row; i++) {
//      curstripe[col+i] = new OECDataPacket(slicesize);
//      code[i] = curstripe[col+i]->_data;
//    }
//    // compute
//    Computation::Multi(code, data, matrix, row, col, slicesize, "Isal");
//
//    // now we free data
//    for (int i=0; i<col; i++) {
//      delete curstripe[i];
//      curstripe[i] = nullptr;
//    }
//    // add the res to writeQueue
//    for (int i=0; i<row; i++) {
//      writeQueue[i]->push(curstripe[col+i]);
//      curstripe[col+i] = nullptr;
//    }
//  }
//
//  // free
//  free(code);
//  free(data);
//  free(curstripe);
//  free(matrix);
//}
//
//void OECWorker::clientReadOffline(string filename,
//                           int filesizeMB,
//                           string poolname,
//                           string stripename) {
//  cout << "OECWorker::clientReadOffline.filename: " << filename << ", filesizeMB: " << filesizeMB << ", poolname: " << poolname << ", stripename: " << stripename << endl;
//  // 0. check integrity
//  FSObjInputStream* objstream = new FSObjInputStream(_conf, filename, _underfs);
//  if (objstream->exist()) {
//    cout << "OECWorker::clientReadOffline." << filename << " exist!" << endl;
//
//    thread readThread = thread([=]{objstream->readObj();});
//    BlockingQueue<OECDataPacket*>* writeQueue = new BlockingQueue<OECDataPacket*>();
//    thread writeThread = thread([=]{writeWorker(writeQueue, filename);});
////    thread writeThread = thread([=]{writeWorker2(writeQueue, filename);});
//
//    while (objstream->hasNext()) {
//      OECDataPacket* curpkt = objstream->dequeue();
//      if (curpkt->_dataLen) writeQueue->push(curpkt);
//    }
//
//    // endpkt
//    OECDataPacket* endpkt = new OECDataPacket(0);
//    writeQueue->push(endpkt);
//   
//    readThread.join();
//    writeThread.join();
//    if (writeQueue) delete writeQueue;
//    if (objstream) delete objstream;
//  } else {
//    cout << "OECWorker::clientReadOffline." << filename << " corrupted!" << endl;
//    if (objstream) delete objstream;
//    // 1. send request to coordinator for degraded read.
//    struct timeval coor1, coor2;
//    gettimeofday(&coor1, NULL);
//    CoorCommand* coorCmd = new CoorCommand();
//    coorCmd->buildType5(5, _conf->_localIp, filename, poolname, stripename);
////    coorCmd->sendTo(_conf->_coorIp);
//    coorCmd->sendTo(_coorCtx);
//  
//    delete coorCmd;
//
//    // 2. wait for instruction from coordinator
//    string wkey = "degraded:"+filename;
//    redisContext* waitCtx = RedisUtil::createContext("127.0.0.1");
//    redisReply* rReply = (redisReply*)redisCommand(waitCtx, "blpop %s 0", wkey.c_str());
//    char* response = rReply -> element[1] -> str;
//    // 2.1 corrupt idx
//    int sid;
//    memcpy((char*)&sid, response, 4); response += 4;
//    sid = ntohl(sid);
//    cout << "OECWorker::clientReadOffline.sid: " << sid;
//    // 2.2 scratio
//    int scratio;
//    memcpy((char*)&scratio, response, 4); response += 4;
//    scratio = ntohl(scratio);
//    cout << ", scratio: " << scratio;
//    // 2.3 num of pkt
//    int num;
//    memcpy((char*)&num, response, 4); response += 4;
//    num = ntohl(num);
//    cout << ", num: " << num;
//    // 2.4. loclist
//    vector<unsigned int> loclist;
//    cout << ", loc:";
//    for (int i=0; i<scratio; i++) {
//      unsigned int ip;
//      memcpy((char*)&ip, response, 4); response += 4;
//      ip = ntohl(ip);
//      cout << " " << RedisUtil::ip2Str(ip);
//      loclist.push_back(ip);
//    }
//    cout << endl;
//    freeReplyObject(rReply);
//    gettimeofday(&coor2, NULL);
//    cout << "OECWorker::clientReadOffline.get metadata from coordinator: " << RedisUtil::duration(coor1, coor2) << endl;
//    redisFree(waitCtx);
//
//    // 3. create blockingqueue
//    BlockingQueue<OECDataPacket*>** fetchQueue = (BlockingQueue<OECDataPacket*>**)calloc(scratio, sizeof(BlockingQueue<OECDataPacket*>*));
//    for (int i=0; i<scratio; i++) {
//      fetchQueue[i] = new BlockingQueue<OECDataPacket*>();
//    }
//
//    BlockingQueue<OECDataPacket*>* writeQueue = new BlockingQueue<OECDataPacket*>();
//
//    // 4. create fetchThread
//    vector<thread> fetchThreads = vector<thread>(scratio);
//    for (int i=0; i<scratio; i++) {
//      int cid = sid*scratio+i;
//      string keybase = stripename+":"+to_string(cid);
//      fetchThreads[i] = thread([=]{fetchWorker(fetchQueue[i], keybase, loclist[i], num);});
//    } 
//
//    // 5. create writeThread
//    thread writeThread = thread([=]{writeWorker(writeQueue, filename);});
////    thread writeThread = thread([=]{writeWorker2(writeQueue, filename);});
//
//    // 6. fetch pkt from fetchQueue to writeQueue
//    for (int i=0; i<num; i++) {
//      if (scratio == 1) {
//        OECDataPacket* curpkt = fetchQueue[0]->pop();
//        writeQueue->push(curpkt);
//      } else {
//        int slicesize = _conf->_pktSize/scratio;
//        char* content = (char*)calloc(4+_conf->_pktSize, sizeof(char));
//        
//        int tmplen = htonl(_conf->_pktSize);
//        memcpy(content, (char*)&tmplen, 4);
//        for (int j=0; j<scratio; j++) {
//          OECDataPacket* curpkt = fetchQueue[j]->pop();
//          memcpy(content+4+j*slicesize, curpkt->_data, slicesize);
//          delete curpkt;
//        }
//        OECDataPacket* retpkt = new OECDataPacket();
//        retpkt->setRaw(content);
//        writeQueue->push(retpkt);
//      }
//    }
//
//    // 7. endpkt
//    OECDataPacket* endpkt = new OECDataPacket(0);
//    writeQueue->push(endpkt);
//
//    // last. join and delete
//    for (int i=0; i<scratio; i++) fetchThreads[i].join();
//    writeThread.join();
//    for (int i=0; i<scratio; i++) delete fetchQueue[i];
//    free(fetchQueue);
//    delete writeQueue;
//  }
//  cout << "OECWorker::clientReadOffline finishes!" << endl;
//}
