#include "ECTask.hh"

ECTask::ECTask() {
  _taskCmd = (char*)calloc(MAX_COMMAND_LEN, sizeof(char));
  _cmLen = 0;
}

ECTask::~ECTask() {
  if (_taskCmd) {
    free(_taskCmd);
    _taskCmd = 0;
  }
  _cmLen = 0;
}

ECTask::ECTask(char* reqStr) {
  _taskCmd = reqStr;
  _cmLen = 0;

  // parse type
  _type = readInt();

  switch(_type) {
    case 2: resolveType2(); break;
    default: break;
  }
  _taskCmd = nullptr;
  _cmLen = 0;
}

void ECTask::setType(int type) {
  _type = type;
}

void ECTask::addIdx(int idx) {
  _indices.push_back(idx);
}

void ECTask::setChildren(vector<int> children) {
  _children = children;
}

void ECTask::setCoefmap(unordered_map<int, vector<int>> map) {
  _coefMap = map;
}

void ECTask::setPersistDSS(int pdss) {
  _persistDSS = pdss;
}

void ECTask::addRef(unordered_map<int, int> ref) {
  for (auto item: ref) {
    _refNum.insert(item);
  }
}

void ECTask::addRef(int idx, int ref) {
  _refNum.insert(make_pair(idx, ref));
}

//void ECTask::setBind(int id) {
//  _bind = id;
//}

vector<int> ECTask::getIndices() {
  return _indices;
}

vector<int> ECTask::getChildren() {
  return _children;
}

unordered_map<int, vector<int>> ECTask::getCoefMap() {
  return _coefMap;
}

int ECTask::getPersistType() {
  return _persistDSS;
}

unordered_map<int, int> ECTask::getRefMap() {
  return _refNum;
}

void ECTask::writeInt(int value) {
  int tmpv = htonl(value);
  memcpy(_taskCmd + _cmLen, (char*)&tmpv, 4); _cmLen += 4;
}

void ECTask::writeString(string s) {
  int slen = s.length();
  int tmpslen = htonl(slen);
  // string length
  memcpy(_taskCmd + _cmLen, (char*)&tmpslen, 4); _cmLen += 4;
  // string
  memcpy(_taskCmd + _cmLen, s.c_str(), slen); _cmLen += slen;
}

int ECTask::readInt() {
  int tmpint;
  memcpy((char*)&tmpint, _taskCmd + _cmLen, 4); _cmLen += 4;
  return ntohl(tmpint);
}

string ECTask::readString() {
  string toret;
  int slen = readInt();
  char* sname = (char*)calloc(sizeof(char), slen+1);
  memcpy(sname, _taskCmd + _cmLen, slen); _cmLen += slen;
  toret = string(sname);
  free(sname);
  return toret;
}

void ECTask::buildType2() {
  writeInt(_type);
  writeInt(_children.size());
  for (int i=0; i<_children.size(); i++) writeInt(_children[i]);
  writeInt(_coefMap.size());
  for (auto item: _coefMap) {
    int target = item.first;
    vector<int> coef = item.second;
    writeInt(target);
    writeInt(coef.size());
    for (int i=0; i<coef.size(); i++) writeInt(coef[i]);
  }
}

void ECTask::resolveType2() {
  int numchildren = readInt();
  for (int i=0; i<numchildren; i++) {
    int curidx = readInt();
    _children.push_back(curidx);
  }
  int targetnum = readInt();
  for (int i=0; i<targetnum; i++) {
    int target = readInt();
    int coefnum = readInt();
    vector<int> coef;
    for (int j=0; j<coefnum; j++) {
      int curc = readInt();
      coef.push_back(curc);
    }
    _coefMap.insert(make_pair(target, coef));
  }
}

void ECTask::sendTo(string key, unsigned int ip) {
  redisContext* sendCtx = RedisUtil::createContext(ip);
  redisReply* rReply = (redisReply*)redisCommand(sendCtx, "RPUSH %s %b", key.c_str(), _taskCmd, _cmLen);
  freeReplyObject(rReply);
  redisFree(sendCtx);
}

void ECTask::dump() {
  if (_type == 2) {
    for (auto item: _coefMap) {
      int target = item.first;
      vector<int> coef = item.second;
      cout << " Compute: " << target << " = ( ";
      for (int i=0; i<coef.size(); i++) cout << coef[i] << " ";
      cout << ") X ( ";
      for (int i=0; i<coef.size(); i++) cout << _children[i] << " ";
      cout << ")" << endl;
    }
  }
}
