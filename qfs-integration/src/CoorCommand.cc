#include "CoorCommand.h"

namespace KFS
{

using std::string;

CoorCommand::CoorCommand() {
  _coorCmd = (char*)calloc(1024, sizeof(char));
  _cmLen = 0;
  _rKey = "coor_request";
}

CoorCommand::~CoorCommand() {
  if (_coorCmd) free(_coorCmd);
}


void CoorCommand::writeInt(int value) {
  int tmpv = htonl(value);
  memcpy(_coorCmd + _cmLen, (char*)&tmpv, 4); _cmLen += 4;
}

void CoorCommand::writeRawInt(int value) {
  int tmpv = value;
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

void CoorCommand::sendTo(unsigned int ip) {
  redisContext* sendCtx = createContext(ip2Str(ip), 6379);
  redisReply* rReply = (redisReply*)redisCommand(sendCtx, "RPUSH %s %b", _rKey.c_str(), _coorCmd, _cmLen);
  freeReplyObject(rReply);
  redisFree(sendCtx);
}

string CoorCommand::ip2Str(unsigned int ip) {
  struct in_addr addr = {ip};
  return string(inet_ntoa(addr));
}

redisContext* CoorCommand::createContext(string ip, int port) {
  redisContext* retVal = redisConnect(ip.c_str(), port);
  if (retVal == NULL || retVal -> err) {
    if (retVal) {
      std::cerr << "Error: " << retVal -> errstr << std::endl;
      redisFree(retVal);
    } else {
      std::cerr << "redis context creation error" << std::endl;
    }
    throw 1;
  }
  return retVal;
}

void CoorCommand::buildType1(int type, unsigned int clientip, string filename, int replicas) {
    _type = type;
    _clientIp = clientip;
    _fileName = filename;
    _numOfReplicas = replicas;

    writeInt(_type);
    writeRawInt(_clientIp);
    writeString(_fileName);
    writeInt(_numOfReplicas);
}

void CoorCommand::buildType6(int type, unsigned int clientip, string objname) {
    _type = type;
    _clientIp = clientip;
    _fileName = objname;
    
    writeInt(_type);
    writeRawInt(_clientIp);
    writeString(_fileName);
}

void CoorCommand::buildType11(int type, unsigned int clientip, string objname) {
    _type = type;
    _clientIp = clientip;
    _fileName = objname;
    
    writeInt(_type);
    writeRawInt(_clientIp);
    writeString(_fileName);
}

string CoorCommand::waitForLocation(unsigned int ip, string objname) {
  string key = "loc:"+objname;
  redisReply* rReply;
  redisContext* waitCtx = createContext(ip2Str(ip), 6379);
  rReply = (redisReply*)redisCommand(waitCtx, "blpop %s 0", key.c_str());
  char* reqStr = rReply -> element[1] -> str;
  unsigned int loc;
  memcpy((char*)&loc, reqStr, 4);
  return ip2Str(loc);
}

}
