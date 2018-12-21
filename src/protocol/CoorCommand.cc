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
    case 2: resolveType2(); break;
    case 3: resolveType3(); break;
    case 4: resolveType4(); break;
    case 5: resolveType5(); break;
    case 6: resolveType6(); break;
    case 7: resolveType7(); break;
    case 8: resolveType8(); break;
    case 9: resolveType9(); break;
    case 11: resolveType11(); break;
    case 12: resolveType12(); break;
    default: break;
  }
  _coorCmd = nullptr;
  _cmLen = 0;
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

int CoorCommand::readRawInt() {
  int tmpint;
  memcpy((char*)&tmpint, _coorCmd + _cmLen, 4); _cmLen += 4;
  return tmpint;
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

string CoorCommand::getECPoolId() {
  return _ecpoolid;
}

string CoorCommand::getStripeName() {
  return _stripename;
}

int CoorCommand::getOp() {
  return _op;
}

string CoorCommand::getECType() {
  return _ectype;
}

vector<int> CoorCommand::getCorruptIdx() {
  return _corruptIdx;
}

string CoorCommand::getBenchName() {
  return _benchname;
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
  _clientIp = readRawInt();  // This ip is sent from DSS, We test HDFS and find that we should just read raw int.
  _filename = readString();
  _numOfReplicas = readInt();
}

void CoorCommand::buildType2(int type, unsigned int ip, string filename) {
  // set up corresponding parameters
  _type = type;
  _clientIp = ip;
  _filename = filename;

  writeInt(_type);
  writeInt(_clientIp);
  writeString(_filename);
}

void CoorCommand::resolveType2() {
  _clientIp = readInt();
  _filename = readString();
}

void CoorCommand::buildType3(int type,
                             unsigned int ip,
                             string filename) {
  _type = type;
  _clientIp = ip;
  _filename = filename;

  writeInt(_type);
  writeInt(_clientIp);
  writeString(_filename);
}

void CoorCommand::resolveType3() {
  _clientIp = readInt();
  _filename = readString();
}

void CoorCommand::buildType4(int type, unsigned int ip, string poolname, string stripename) {
  _type = type;
  _clientIp = ip;
  _ecpoolid = poolname;
  _stripename = stripename;

  writeInt(_type);
  writeInt(_clientIp);
  writeString(_ecpoolid);
  writeString(_stripename);
}

void CoorCommand::resolveType4() {
  _clientIp = readInt();
  _ecpoolid = readString();
  _stripename = readString();
}

void CoorCommand::buildType5(int type, unsigned int ip, string objname) {
  _type = type;
  _clientIp = ip;
  _filename = objname;

  writeInt(_type);
  writeInt(_clientIp);
  writeString(_filename);
}

void CoorCommand::resolveType5() {
  _clientIp = readInt();
  _filename = readString();
}

void CoorCommand::resolveType6() {
  _clientIp = readInt();
  _filename = readString(); 
}

void CoorCommand::buildType7(int type, int op, string ectype) {
  _type = type;
  _op = op;
  _ectype = ectype;

  writeInt(type);
  writeInt(op);
  writeString(ectype);
}

void CoorCommand::resolveType7() {
  _op = readInt();
  _ectype = readString();
}

void CoorCommand::buildType8(int type, unsigned int ip, string objname) {
  _type = type;
  _clientIp = ip;
  _filename = objname;

  writeInt(_type);
  writeInt(_clientIp);
  writeString(_filename);
}

void CoorCommand::resolveType8() {
  _clientIp = readInt();
  _filename = readString();
}

void CoorCommand::buildType9(int type,
                    unsigned int ip,
                    string filename,
                    vector<int> corruptIdx) {
  _type = type;
  _clientIp = ip;
  _filename = filename;
  _corruptIdx = corruptIdx;

  writeInt(_type);
  writeInt(_clientIp);
  writeString(_filename);
  writeInt(corruptIdx.size());
  for (int i=0; i<corruptIdx.size(); i++) writeInt(corruptIdx[i]);
}

void CoorCommand::resolveType9() {
  _clientIp = readInt();
  _filename = readString();
  int num = readInt();
  for (int i=0; i<num; i++) {
    _corruptIdx.push_back(readInt());
  }
}

void CoorCommand::resolveType11() {
  _clientIp = readInt();
  _filename = readString();
}

void CoorCommand::buildType12(int type,
                              unsigned int ip,
                              string benchname) {
  _type = type;
  _clientIp = ip;
  _benchname = benchname;

  writeInt(_type);
  writeInt(_clientIp);
  writeString(_benchname);
}

void CoorCommand::resolveType12() {
  _clientIp = readInt();
  _benchname = readString();
}

void CoorCommand::dump() {
  cout << "CoorCommand::type: " << _type;
  if (_type == 0) {
    cout << ", client: " << RedisUtil::ip2Str(_clientIp)
         << ", filename: " << _filename << ", ecid: " << _ecid << ", mode: " << _mode 
         << ", filesizeMB: " << _filesizeMB << endl;
  } else if (_type == 1) {
    cout << ", client: " << RedisUtil::ip2Str(_clientIp)
         << ", filename: " << _filename << ", numOfReplicas: " << _numOfReplicas << endl;
  } else if (_type == 2) {
    cout << ", client: " << RedisUtil::ip2Str(_clientIp)
         << ", filename: " << _filename << endl;
  } else if (_type == 3) {
    cout << ", client: " << RedisUtil::ip2Str(_clientIp)
         << ", filename: " << _filename << endl;
  } else if (_type == 4) {
    cout << ", client: " << RedisUtil::ip2Str(_clientIp)
         << ", ecpoolid: " << _ecpoolid
         << ", stripename: " << _stripename << endl;
  } else if (_type == 6) {
    cout << ", client: " << RedisUtil::ip2Str(_clientIp)
         << ", filename: " << _filename << endl;
  } else if (_type == 7) {
    cout << ", enable: " << _op << ", ectype: " << _ectype << endl;
  }
}
