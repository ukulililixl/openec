#include "OECInputStream.hh"

OECInputStream::OECInputStream(Config* conf, 
                               string filename) {
  _conf = conf;
  _filename = filename;
  _localCtx = RedisUtil::createContext(_conf->_localIp);
  init();
}

OECInputStream::~OECInputStream() {
  redisFree(_localCtx);
}

void OECInputStream::init() {
  AGCommand* agCmd = new AGCommand();
  agCmd->buildType1(1, _filename);
  agCmd->sendTo(_conf->_localIp);
  delete agCmd;

  // wait for filesize?
  string wkey = "filesize:"+_filename;
  redisReply* rReply = (redisReply*)redisCommand(_localCtx, "blpop %s 0", wkey.c_str());
  char* response = rReply -> element[1] -> str;
  int tmpfilesize;
  memcpy((char*)&tmpfilesize, response, 4); response += 4;
  _filesizeMB = ntohl(tmpfilesize);

  freeReplyObject(rReply);

  _readQueue = new BlockingQueue<OECDataPacket*>();

  _collectThread = thread([=]{readWorker(_readQueue, _filename);});
}

void OECInputStream::readWorker(BlockingQueue<OECDataPacket*>* readQueue, string keybase) { 
  struct timeval t1, t2, start, end;
  gettimeofday(&start, NULL);

  int pktnum = _filesizeMB * 1048576/_conf->_pktSize;
  redisReply* rReply;
  redisContext* readCtx = _localCtx;

  for (int i=0; i<pktnum; i++) {
    string key = keybase + ":" + to_string(i);
    redisAppendCommand(readCtx, "blpop %s 0", key.c_str());
  }

  double t=0;
  for (int i=0; i<pktnum; i++) {
    string key = keybase+":"+to_string(i);
//    gettimeofday(&t1, NULL);
    redisGetReply(readCtx, (void**)&rReply);
//    gettimeofday(&t2, NULL);
//    if (i == 0) cout << "OECInputStream:: the first pkt : " << RedisUtil::duration(t1, t2) << endl;
//    cout << "OECInputStream::readWorker.getpkt: " << RedisUtil::duration(t1, t2) << endl;
//    t += RedisUtil::duration(t1, t2);
    char* content = rReply->element[1]->str;
    OECDataPacket* pkt = new OECDataPacket(content);
    int curDataLen = pkt->getDatalen();
    readQueue->push(pkt);
    freeReplyObject(rReply);
  }
  gettimeofday(&end, NULL);
  cout << "OECInputStream::readWorker.duration: " << RedisUtil::duration(start, end) << endl;
}

void OECInputStream::output2file(string saveas) {
  struct timeval time1, time2;
  gettimeofday(&time1, NULL);
  ofstream ofs(saveas);
  ofs.close();
  ofs.open(saveas, ios::app);

  int num = _filesizeMB * 1048576 / _conf->_pktSize;

  for (int i=0; i<num; i++) {
    OECDataPacket* curPkt = _readQueue->pop();
    int len = curPkt->getDatalen();
    if (len) {
      ofs.write(curPkt->getData(), len);
    }
    else break;
    delete curPkt;
  }

  ofs.close();
  gettimeofday(&time2, NULL);
  cout << "OECInputStream::output2file.time = " << RedisUtil::duration(time1, time2) << endl;
}

void OECInputStream::close() {
  _collectThread.join();
}
