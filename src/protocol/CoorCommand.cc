#include "CoorCommand.hh"

CoorCommand::CoorCommand() {
  _coorCmd = (char*)calloc(MAX_COMMAND_LEN, sizeof(char));
  _cmLen = 0;
  _rKey = "coor_request";
}

CoorCommand::~CoorCommand() {
  if (_coorCmd) {
    free(_coorCmd);
    _coorCmd = 0;
  }
  _cmLen = 0;
}

CoorCommand::CoorCommand(char* reqStr) {
  _coorCmd = reqStr;
  _cmLen = 0;

  // parse type
  _type = readInt();

  switch(_type) {
    case 0: resolveType0(); break;
    case 1: resolveType1(); break;
    default: break;
  }
}

void CoorCommand::writeInt(int value) {
  int tmpv = htonl(value);
  memcpy(_coorCmd + _cmLen, (char*)&tmpv, 4); _cmLen += 4;
}

void CoorCommand::writeString(string s) {
  int slen = s.length();
  int tmpslen = htonl(slen);
  // string length
  memcpy(_coorCmd + _cmLen, (char*)&tmpslen, 4); _cmLen += 4;
  // string
  memcpy(_coorCmd + _cmLen, s.c_str(), slen); _cmLen += slen;
}

int CoorCommand::readInt() {
  int tmpint;
  memcpy((char*)&tmpint, _coorCmd + _cmLen, 4); _cmLen += 4;
  return ntohl(tmpint);
}

string CoorCommand::readString() {
  string toret;
  int slen = readInt();
  char* sname = (char*)calloc(sizeof(char), slen+1);
  memcpy(sname, _coorCmd + _cmLen, slen); _cmLen += slen;
  toret = string(sname);
  free(sname);
  return toret;
}

int CoorCommand::getType() {
  return _type;
}

unsigned int CoorCommand::getClientip() {
  return _clientIp;
}

string CoorCommand::getFilename() {
  return _filename;
}

string CoorCommand::getEcid() {
  return _ecid;
}

int CoorCommand::getMode() {
  return _mode;
}

int CoorCommand::getFilesizeMB() {
  return _filesizeMB;
}

int CoorCommand::getNumOfReplicas() {
  return _numOfReplicas;
}

void CoorCommand::sendTo(unsigned int ip) {
  redisContext* sendCtx = RedisUtil::createContext(ip);
  redisReply* rReply = (redisReply*)redisCommand(sendCtx, "RPUSH %s %b", _rKey.c_str(), _coorCmd, _cmLen);
  freeReplyObject(rReply);
  redisFree(sendCtx);
}

void CoorCommand::sendTo(redisContext* sendCtx) {
  redisReply* rReply = (redisReply*)redisCommand(sendCtx, "RPUSH %s %b", _rKey.c_str(), _coorCmd, _cmLen);
  freeReplyObject(rReply);
  redisFree(sendCtx);
}

void CoorCommand::buildType0(int type, unsigned int ip, string filename, string ecid, int mode, int filesizeMB) {
  // set up corresponding parameters
  _type = type;
  _clientIp = ip;
  _filename = filename;
  _ecid = ecid;
  _mode = mode;
  _filesizeMB = filesizeMB;

  // 1. type
  writeInt(_type);
  // 2. ipo
  writeInt(_clientIp);
  // 3. filename
  writeString(_filename);
  // 4. ecid
  writeString(_ecid);
  // 5. mode
  writeInt(_mode);
  // 6. filesizeMB
  writeInt(_filesizeMB);
}

void CoorCommand::resolveType0() {
  // 2. ip
  _clientIp = readInt();
  // 3. filename
  _filename = readString();
  // 4. ecid
  _ecid = readString();
  // 5. mode
  _mode = readInt();
  // 6. filesizeMB
  _filesizeMB = readInt();
}

void CoorCommand::resolveType1() {
  _clientIp = readInt();
  _filename = readString();
  _numOfReplicas = readInt();
}

void CoorCommand::dump() {
  if (_type == 0) {
    cout << "CoorCommand::type: " << _type << ", client: " << RedisUtil::ip2Str(_clientIp) 
         << ", filename: " << _filename << ", ecid: " << _ecid << ", mode: " << _mode 
         << ", filesizeMB: " << _filesizeMB << endl;
  } else if (_type == 1) {
    cout << "CoorCommand::type: " << _type << ", client: " <<  RedisUtil::ip2Str(_clientIp)
         << ", filename: " << _filename << ", numOfReplicas: " << _numOfReplicas << endl;
  }
}
