#include "FSObjOutputStream.hh"

FSObjOutputStream::FSObjOutputStream(Config* conf, string objname, UnderFS* fs, int pktnum) {
  _conf = conf;
  _objname = objname;
  _totalPktNum = pktnum;

  _queue = new BlockingQueue<OECDataPacket*>();
  _dataPktNum = 0;
  _finish = false;
  _objsize = 0;

  // connect to hdfs3
  struct timeval time1, time2;
  gettimeofday(&time1, NULL);
  _underfs = fs;
  _underfile = _underfs->openFile(objname, "write");
  if (!_underfile) {
    cout << "ERROR::FSObjOutputStream fail to connect to hdfs!" << endl;
    exit(-1);
  }
  gettimeofday(&time2, NULL);
//  cout << "FSObjOutputStream.connect to hdfs.time = " << RedisUtil::duration(time1, time2) << endl;
  cout << "FSObjOutputStream.objname: " << objname << ", pktnum: " << _totalPktNum << endl;
}

FSObjOutputStream::~FSObjOutputStream() {
  if (_underfile) {
    _underfs->closeFile(_underfile);
    _underfile = NULL;
  }
  if (_queue) delete _queue;
}

void FSObjOutputStream::writeObj() {
  struct timeval time1, time2, time3;
  gettimeofday(&time1, NULL);
  int pktid = 0;

  for (int pktid=0; pktid < _totalPktNum; pktid++) {
    OECDataPacket* curPkt = _queue->pop();
//    cout << "FSObjOutputStream("<<_objname<<")::write pkt " << pktid << ", len = " << curPkt->getDatalen() << endl;
    _objsize += curPkt->getDatalen();
    // write to hdfs
    _underfs->writeFile(_underfile, curPkt->getData(), curPkt->getDatalen());
    _underfs->flushFile(_underfile);
    delete curPkt;
  }

  gettimeofday(&time2, NULL);
  cout << "FSObjOutputStream.writeObj " << _objname << ".writeFileTime: " << RedisUtil::duration(time1, time2) << endl;
  _finish = true;
}

void FSObjOutputStream::enqueue(OECDataPacket* pkt) {
  _queue->push(pkt);
  _dataPktNum++;
}

bool FSObjOutputStream::getFinish() {
  return _finish;
}

BlockingQueue<OECDataPacket*>* FSObjOutputStream::getQueue() {
  return _queue;
}
