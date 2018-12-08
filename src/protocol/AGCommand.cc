#include "AGCommand.hh"

AGCommand::AGCommand() {
  _agCmd = (char*)calloc(MAX_COMMAND_LEN, sizeof(char));
  _cmLen = 0;
  _rKey = "ag_request";
}

AGCommand::~AGCommand() {
  if (_agCmd) {
    free(_agCmd);
    _agCmd = 0;
  }
  _cmLen = 0;
}

AGCommand::AGCommand(char* reqStr) {
  _agCmd = reqStr;
  _cmLen = 0; 

  // parse type
  _type = readInt();

  switch(_type) {
    case 0: resolveType0(); break;
    case 1: resolveType1(); break;
    case 2: resolveType2(); break;
    case 3: resolveType3(); break;
    case 5: resolveType5(); break;
    case 7: resolveType7(); break;
    case 10: resolveType10(); break;
    case 11: resolveType11(); break;
    default: break;
  }
  _agCmd = nullptr;
  _cmLen = 0;
}

void AGCommand::writeInt(int value) {
  int tmpv = htonl(value);
  memcpy(_agCmd + _cmLen, (char*)&tmpv, 4); _cmLen += 4;
}

void AGCommand::writeString(string s) {
  int slen = s.length();
  int tmpslen = htonl(slen);
  // string length
  memcpy(_agCmd + _cmLen, (char*)&tmpslen, 4); _cmLen += 4;
  // string
  memcpy(_agCmd + _cmLen, s.c_str(), slen); _cmLen += slen;
}

int AGCommand::readInt() {
  int tmpint;
  memcpy((char*)&tmpint, _agCmd + _cmLen, 4); _cmLen += 4;
  return ntohl(tmpint);
}

string AGCommand::readString() {
  string toret;
  int slen = readInt();
  char* sname = (char*)calloc(sizeof(char), slen+1);
  memcpy(sname, _agCmd + _cmLen, slen); _cmLen += slen;
  toret = string(sname);
  free(sname);
  return toret;
}

int AGCommand::getType() {
  return _type;
}

char* AGCommand::getCmd() {
  return _agCmd;
}

int AGCommand::getCmdLen() {
  return _cmLen;
}

string AGCommand::getFilename() {
  return _filename;
}

string AGCommand::getEcid() {
  return _ecid;
}

string AGCommand::getMode() {
  return _mode;
}

int AGCommand::getFilesizeMB() {
  return _filesizeMB;
}

bool AGCommand::getShouldSend() {
  return _shouldSend;
}

unsigned int AGCommand::getSendIp() {
  return _sendIp;
}

string AGCommand::getStripeName() {
  return _stripeName;
}

int AGCommand::getNum() {
  return _num;
}

string AGCommand::getReadObjName() {
  return _readObjName;
}

vector<int> AGCommand::getReadCidList() {
  return _readCidList;
}

unordered_map<int, int> AGCommand::getCacheRefs() {
  return _cacheRefs;
}

int AGCommand::getNprevs() {
  return _nprevs;
}

vector<int> AGCommand::getPrevCids() {
  return _prevCids;
}

vector<unsigned int> AGCommand::getPrevLocs() {
  return _prevLocs;
}

unordered_map<int, vector<int>> AGCommand::getCoefs() {
  return _coefs;
}

string AGCommand::getWriteObjName() {
  return _writeObjName;
}

int AGCommand::getN() {
  return _ecn;
}

int AGCommand::getK() {
  return _eck;
}

int AGCommand::getW() {
  return _ecw;
}

int AGCommand::getComputen() {
  return _computen;
}

int AGCommand::getObjnum() {
  return _objnum;
}

int AGCommand::getBasesizeMB() {
  return _basesizeMB;
}

void AGCommand::setRkey(string key) {
  _rKey = key;
} 

void AGCommand::sendTo(unsigned int ip) {
  redisContext* sendCtx = RedisUtil::createContext(ip);
  redisReply* rReply = (redisReply*)redisCommand(sendCtx, "RPUSH %s %b", _rKey.c_str(), _agCmd, _cmLen);
  freeReplyObject(rReply);
  redisFree(sendCtx);
}

void AGCommand::buildType0(int type,
                           string filename,
                           string ecid,
                           string mode, 
                           int filesizeMB) {
  // set up corresponding parameters
  _type = type;
  _filename = filename;
  _ecid = ecid;
  _mode = mode;
  _filesizeMB = filesizeMB;

  // 1. type
  writeInt(_type);
  // 2. filename
  writeString(_filename);
  // 3. ecid
  writeString(_ecid);
  // 4. mode
  writeString(_mode);
  // 5. filesizeMB
  writeInt(_filesizeMB);
}

void AGCommand::resolveType0() {
  // 2. filename
  _filename = readString();
  // 3. ecid
  _ecid = readString();
  // 4. mode
  _mode = readString();
  // 5. filesizeMB
  _filesizeMB = readInt();
}

void AGCommand::buildType1(int type,
                           string filename) {
  _type = type;
  _filename = filename;

  writeInt(_type);
  writeString(_filename);
}

void AGCommand::resolveType1() {
  _filename = readString();
}

void AGCommand::buildType2(int type,
                     unsigned int sendIp,
                     string stripeName,
                     int w,
                     int numslices,
                     string readObjName,
                     vector<int> cidlist,
                     unordered_map<int, int> ref) {
  _shouldSend = true;
  _type = type;
  _sendIp = sendIp;
  _stripeName = stripeName;
  _ecw = w;
  _num = numslices;
  _readObjName = readObjName;
  _readCidList = cidlist;
  for (auto item:ref) {
    _cacheRefs.insert(item);
  }

  writeInt(_type);
  writeString(_stripeName);
  writeInt(_ecw);
  writeInt(_num);
  writeString(_readObjName);
  writeInt(cidlist.size());
  for (int i=0; i<cidlist.size(); i++) {
    int id = cidlist[i];
    writeInt(id);
    writeInt(ref[id]);
  }
}

void AGCommand::resolveType2() {
  _stripeName = readString();
  _ecw = readInt();
  _num = readInt();
  _readObjName = readString();
  int listsize = readInt();
  for (int i=0; i<listsize; i++) {
    int id = readInt();
    int ref = readInt();
    _readCidList.push_back(id);
    _cacheRefs.insert(make_pair(id, ref));
  }
}

void AGCommand::buildType3(int type,
                    unsigned int sendIp,
                    string stripeName,
                    int w,
                    int num,
                    int prevnum,
                    vector<int> prevCids,
                    vector<unsigned int> prevLocs,
                    unordered_map<int, vector<int>> coefs,
                    unordered_map<int, int> ref) {
  _shouldSend = true;
  _type = type;
  _sendIp = sendIp;
  _stripeName = stripeName;
  _ecw = w;
  _num = num;
  _nprevs = prevnum;
  _prevCids = prevCids;
  _prevLocs = prevLocs;
  _coefs = coefs;
  for (auto item:ref) {
  _cacheRefs.insert(item);
  }

  writeInt(_type);
  writeString(_stripeName);
  writeInt(_ecw);
  writeInt(_num);
  writeInt(_nprevs);
  for (int i=0; i<_nprevs; i++) {
    writeInt(_prevCids[i]);
    writeInt(_prevLocs[i]);
  }
  int targetnum = _coefs.size();
  writeInt(targetnum);
  for (auto item: _coefs) {
    int target = item.first;
    vector<int> coef = item.second;
    int r = ref[target];
    writeInt(target);
    for (int i=0; i<_nprevs; i++) writeInt(coef[i]);
    writeInt(r);
  }
}

void AGCommand::resolveType3() {
  _stripeName = readString();
  _ecw = readInt();
  _num = readInt();
  _nprevs = readInt();
  for (int i=0; i<_nprevs; i++) {
    _prevCids.push_back(readInt());
    _prevLocs.push_back(readInt());
  }
  int targetnum = readInt();
  for (int i=0; i<targetnum; i++) {
    int target = readInt();
    vector<int> coef;
    for (int j=0; j<_nprevs; j++) coef.push_back(readInt());
    int r = readInt();
    _coefs.insert(make_pair(target, coef));
    _cacheRefs.insert(make_pair(target, r));
  }
}

void AGCommand::buildType5(int type,
                    unsigned int sendIp,
                    string stripename,
                    int w,
                    int num,
                    int prevnum,
                    vector<int> prevCids,
                    vector<unsigned int> prevLocs,
                    string writeobjname) {
  _shouldSend = true;
  _type = type;
  _sendIp = sendIp;
  _stripeName = stripename;
  _ecw = w;
  _num = num;
  _nprevs = prevnum;
  _prevCids = prevCids;
  _prevLocs = prevLocs;
  _writeObjName = writeobjname;

  writeInt(_type);
  writeString(_stripeName);
  writeInt(_ecw);
  writeInt(_num);
  writeInt(_nprevs);
  for (int i=0; i<_nprevs; i++) {
    writeInt(_prevCids[i]);
    writeInt(_prevLocs[i]);
  }
  writeString(_writeObjName);
}

void AGCommand::resolveType5() {
  _stripeName = readString();
  _ecw = readInt();
  _num = readInt();
  _nprevs = readInt();
  for (int i=0; i<_nprevs; i++) {
    _prevCids.push_back(readInt());
    _prevLocs.push_back(readInt());
  }
  _writeObjName = readString();
}

void AGCommand::buildType7(int type,
                    unsigned int sendIp,
                    string stripename,
                    int w,
                    int num,
                    string readObjName,
                    vector<int> cidlist,
                    int prevnum,
                    vector<int> prevCids,
                    vector<unsigned int> prevLocs,
                    unordered_map<int, vector<int>> coefs,
                    unordered_map<int, int> ref) {
  _shouldSend = true;
  _type = type;
  _sendIp = sendIp;
  _stripeName = stripename;
  _ecw = w;
  _num = num;
  _nprevs = prevnum;
  _readObjName = readObjName;
  _readCidList = cidlist;
  _nprevs = prevnum;
  _prevCids = prevCids;
  _prevLocs = prevLocs;
  _coefs = coefs;
  _cacheRefs = ref;

  writeInt(_type);
  writeString(_stripeName);
  writeInt(_ecw);
  writeInt(_num);
  writeString(_readObjName);
  writeInt(_readCidList.size());
  for (int i=0; i<_readCidList.size(); i++) writeInt(_readCidList[i]);
  writeInt(_nprevs);
  for (int i=0; i<_nprevs; i++) {
    writeInt(_prevCids[i]);
    writeInt(_prevLocs[i]);
  }
  writeInt(_coefs.size());
  for (auto item: _coefs) {
    writeInt(item.first);
    for (int i=0; i<_nprevs; i++) writeInt(item.second[i]);
  }
  writeInt(ref.size());
  for (auto item: ref) {
    writeInt(item.first);
    writeInt(item.second);
  }
}

void AGCommand::resolveType7() {
  _stripeName = readString();
  _ecw = readInt();
  _num = readInt();
  _readObjName = readString();
  int readnum = readInt();
  for (int i=0; i<readnum; i++) _readCidList.push_back(readInt());
  _nprevs = readInt();
  for (int i=0; i<_nprevs; i++) {
    _prevCids.push_back(readInt());
    _prevLocs.push_back(readInt());
  }
  int targetnum = readInt();
  for (int i=0; i<targetnum; i++) {
    int target = readInt();
    vector<int> coef;
    for (int i=0; i<_nprevs; i++) coef.push_back(readInt());
    _coefs.insert(make_pair(target, coef));
  }
  int refnum = readInt();
  for (int i=0; i<refnum; i++) {
    int cid = readInt();
    int r = readInt();
    _cacheRefs.insert(make_pair(cid, r));
  }
}

void AGCommand::buildType10(int type,
                            int ecn,
                            int eck,
                            int ecw,
                            int computen) {
  // set up corresponding parameters
  _type = type;
  _ecn = ecn;
  _eck = eck;
  _ecw = ecw;
  _computen = computen;
  
  writeInt(_type);
  writeInt(_ecn);
  writeInt(_eck);
  writeInt(_ecw);
  writeInt(_computen);
}

void AGCommand::resolveType10() {
  _ecn = readInt();
  _eck = readInt();
  _ecw = readInt();
  _computen = readInt();
}

void AGCommand::buildType11(int type,
                            int objnum,
                            int basesizeMB) {
  _type = type;
  _objnum = objnum;
  _basesizeMB = basesizeMB;

  writeInt(_type);
  writeInt(_objnum);
  writeInt(_basesizeMB);
}

void AGCommand::resolveType11() {
  _objnum = readInt();
  _basesizeMB = readInt();
}

void AGCommand::dump() {
  if (_type == 0) {
    cout << "AGCommand::clientWrite: " << _filename << ", ecid: " << _ecid << ", mode: " << _mode << ", size: " << _filesizeMB << endl;
  } else if (_type == 1) {
    cout << "AGCommand::clientRead: " << _filename << endl;
  } else if (_type == 2) {
    cout << "AGCommand::Load, ip: " << RedisUtil::ip2Str(_sendIp) << " objname: " << _readObjName << ", cidlist: ";
    for (int i=0; i<_readCidList.size(); i++) cout << _readCidList[i] << " ";
    cout << ", write: ";
    for (auto item: _cacheRefs) {
      cout << item.first << " -> " << item.second << ", ";
    }
    cout << endl;
  } else if (_type == 3) {
    cout << "AGCommand::FetchAndCompute, ip: " << RedisUtil::ip2Str(_sendIp) << endl;
    for (int i=0; i<_nprevs; i++) {
      cout << "    Fetch: " << _prevCids[i] << " from " << RedisUtil::ip2Str(_prevLocs[i]) << endl;
    }
    for (auto item: _coefs) {
      int target = item.first;
      vector<int> coef = item.second;
      cout << "    Compute: " << target << ", coef: ";
      for (int i=0; i<coef.size(); i++) cout << coef[i] << " ";
      cout << ", cache: " << _cacheRefs[target] << endl;
    }
  } else if (_type == 5) {
    cout << "AGCommand::FetchAndPersist, ip: " << RedisUtil::ip2Str(_sendIp) << endl;
    for (int i=0; i<_nprevs; i++) {
      cout << "    Fetch: " << _prevCids[i] << " from " << RedisUtil::ip2Str(_prevLocs[i]) << endl;
    }
    cout << "    Persist as " << _writeObjName << endl;
  } else if (_type == 7) {
    cout << "AGCommand::ReadFetchComputeAndCache, ip: " << RedisUtil::ip2Str(_sendIp) << endl;
    cout << "    Read: objname: " << _readObjName << ", cidlist: ";
    for (int i=0; i<_readCidList.size(); i++) cout << _readCidList[i] << " ";
    cout << endl;
    for (int i=0; i<_nprevs; i++) {
      cout << "    Fetch: " << _prevCids[i] << " from " << RedisUtil::ip2Str(_prevLocs[i]) << endl;
    }
    for (auto item: _coefs) {
      int target = item.first;
      vector<int> coef = item.second;
      cout << "    Compute: " << target << ", coef: ";
      for (int i=0; i<coef.size(); i++) cout << coef[i] << " ";
    }
    for (auto item: _cacheRefs) {
      cout << "    Cache: " << item.first << " : " << item.second << endl;
    }
  }
}
