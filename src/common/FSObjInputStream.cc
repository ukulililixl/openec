#include "FSObjInputStream.hh"

FSObjInputStream::FSObjInputStream(Config* conf, string objname, UnderFS* fs) {
  struct timeval time1, time2;
  gettimeofday(&time1, NULL);
  _conf = conf;
  _objname = objname;
  _queue = new BlockingQueue<OECDataPacket*>();
  _dataPktNum = 0;

  _underfs = fs;
  _underfile = _underfs->openFile(objname, "read");
  if (!_underfile) {
    _exist = false;
  } else {
//    _exist = true;
    _objbytes = _underfs->getFileSize(_underfile);
    cout << "FSObjInputStream::constructor.objsize = " << _objbytes << endl;
    _offset = 0;
    if (_objbytes == 0) _exist = false;
    else _exist = true;
  }
  gettimeofday(&time2, NULL);
  cout << "FSObjInputStream::constructor.time = " << RedisUtil::duration(time1, time2) << endl;
}

FSObjInputStream::~FSObjInputStream() {
  if (_queue) delete _queue;
  if (_underfile) _underfs->closeFile(_underfile);
}

void FSObjInputStream::readObj(int slicesize) {
  struct timeval time1, time2;
  gettimeofday(&time1, NULL);
  while(true) {
    int hasread = 0;
    char* buf = (char*)calloc(slicesize+4, sizeof(char));
    if (!buf) {
      cout << "FSObjInputStream::readObj.malloc buffer fail" << endl;
      return;
    }
    while(hasread < slicesize) {
      int len = _underfs->readFile(_underfile, buf+4+hasread, slicesize-hasread);
      if (len == 0) break;
      hasread += len;
    }

    // set hasread in the first 4 bytes of buf
    int tmplen = htonl(hasread);
    memcpy(buf, (char*)&tmplen, 4);

    if (hasread) {
      OECDataPacket* curPkt = new OECDataPacket();
      curPkt->setRaw(buf);
      _queue->push(curPkt); _dataPktNum++;
    }
    if (hasread <= 0) break;
  }
  gettimeofday(&time2, NULL);
  cout << "FSObjInputStream.readObj.duration = " << RedisUtil::duration(time1, time2) << " for " << _objname << " of " << _dataPktNum << " slices"<< endl;
}

void FSObjInputStream::readObj() {

  struct timeval time1, time2;
  gettimeofday(&time1, NULL);
  while(true) {
    int hasread = 0;
    char* buf = (char*)calloc(_conf->_pktSize+4, sizeof(char));
    if (!buf) {
      cout << "FSObjInputStream::readObj.malloc buffer fail" << endl;
      return;
    }
    while(hasread < _conf->_pktSize) {
      int len = _underfs->readFile(_underfile, buf+4+hasread, _conf->_pktSize-hasread);
      if (len == 0) break;
      hasread += len;
    }

    // set hasread in the first 4 bytes of buf
    int tmplen = htonl(hasread);
    memcpy(buf, (char*)&tmplen, 4);

    if (hasread) {
      OECDataPacket* curPkt = new OECDataPacket();
      curPkt->setRaw(buf);
      _queue->push(curPkt); _dataPktNum++;
    }
    if (hasread <= 0) break;
  }
  gettimeofday(&time2, NULL);
  cout << "FSObjInputStream.readObj.duration = " << RedisUtil::duration(time1, time2) << " for " << _objname << ", pktnum: " << _dataPktNum << endl;
}

void FSObjInputStream::readObj(int w, vector<int> list, int slicesize) {
  if (w == 1) {
    readObj();
    return;
  }

  struct timeval time1, time2;
  gettimeofday(&time1, NULL);
  // we first transfer items in list %w
  vector<int> offsetlist;
  for (int i=0; i<list.size(); i++) offsetlist.push_back(list[i]%w);
  sort(offsetlist.begin(), offsetlist.end());

  // for each w slices, we put those slice whose index is in offsetlist
  int stripeid=0;
  int pktsize = _conf->_pktSize;
  int stripenum = _objbytes / pktsize;
  cout << "FSObjInputStream::readObj.stripenum:  " << stripenum << endl;
  int slicenum = 0;
  while (stripeid < stripenum) {
    int start = stripeid * pktsize;
    for (int i=0; i<offsetlist.size(); i++) {
      int offidx = offsetlist[i];
      int slicestart = start + offidx * slicesize;
      char* buf = (char*)calloc(slicesize+4, sizeof(char));
  
      int hasread = 0;
      while(hasread < slicesize) {
        int len = _underfs->pReadFile(_underfile, slicestart + hasread, buf+4 + hasread, slicesize - hasread);
        if (len == 0)break;
        hasread += len;
      }

      // set hasread in the first 4 bytes of buf
      int tmplen = htonl(hasread);
      memcpy(buf, (char*)&tmplen, 4);

      if (hasread) {
        OECDataPacket* curPkt = new OECDataPacket();
        curPkt->setRaw(buf);
        _queue->push(curPkt); slicenum++;
      }
    }
    stripeid++;
  }
  gettimeofday(&time2, NULL);
  cout << "FSObjInputStream.readObj.duration = " << RedisUtil::duration(time1, time2) << " for " << _objname << ", totally " << slicenum << "slices" << endl;
}

void FSObjInputStream::readObj(int slicesize, int unitIdx) {

  if (slicesize == _conf->_pktSize) {
    readObj();
    return;
  }

  struct timeval time1, time2;
  gettimeofday(&time1, NULL);
  int pktnum = 0;
  while(true) {
    int hasread = 0;
    long objoffset = pktnum * _conf->_pktSize + unitIdx * slicesize;

    char* buf = (char*)calloc(slicesize+4, sizeof(char));
    if (!buf) {
      cout << "FSObjInputStream::readObj.malloc buffer fail" << endl;
      return;
    }

    while(hasread < slicesize) {
      int len = _underfs->pReadFile(_underfile, objoffset + hasread, buf+4 + hasread, slicesize - hasread);
      if (len == 0) break;
      hasread += len;
    }

    // set hasread in the first 4 bytes of buf
    int tmplen = htonl(hasread);
    memcpy(buf, (char*)&tmplen, 4);

    if (hasread) {
      OECDataPacket* curPkt = new OECDataPacket();
      curPkt->setRaw(buf);
      _queue->push(curPkt); pktnum++;
    }

    if (hasread <= 0) break;
  }

  gettimeofday(&time2, NULL);
  cout << "FSObjInputStream.readObj.duration = " << RedisUtil::duration(time1, time2) << " for " << _objname << ", pktnum = " << pktnum << endl;
}

OECDataPacket* FSObjInputStream::dequeue() {
  OECDataPacket* toret = _queue->pop();
  _offset += toret->getDatalen();
  return toret;
}

bool FSObjInputStream::exist() {
  return _exist;
}

bool FSObjInputStream::hasNext() {
  return (_offset < _objbytes) ? true:false;
}

int FSObjInputStream::pread(long objoffset, char* buffer, int buflen) {
  int hasread = 0;
  while (hasread < buflen) {
    int len = _underfs->pReadFile(_underfile, objoffset + hasread, buffer+hasread, buflen - hasread);
    hasread += len;
  }
  return hasread;
}

BlockingQueue<OECDataPacket*>* FSObjInputStream::getQueue() {
  return _queue;
}
