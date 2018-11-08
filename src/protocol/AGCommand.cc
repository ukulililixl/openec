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
  }
}
