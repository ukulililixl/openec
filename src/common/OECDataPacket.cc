#include "OECDataPacket.hh"

OECDataPacket::OECDataPacket() {
}

OECDataPacket::OECDataPacket(char* raw) {
  int tmplen;
  memcpy((char*)&tmplen, raw, 4);
  _dataLen = ntohl(tmplen);

  _raw = (char*)calloc(_dataLen+4, sizeof(char));
  memcpy(_raw, raw, _dataLen+4);

  _data = _raw+4;
}

OECDataPacket::OECDataPacket(int len) {
  _raw = (char*)calloc(len+4, sizeof(char));
  _data = _raw+4;
  _dataLen = len;

  int tmplen = htonl(len) ;
  memcpy(_raw, (char*)&tmplen, 4);
}

OECDataPacket::~OECDataPacket() {
  if (_raw) free(_raw);
}

void OECDataPacket::setRaw(char* raw) {
  int tmplen;
  memcpy((char*)&tmplen, raw, 4);
  _dataLen = ntohl(tmplen);

  _raw = raw;
  _data = _raw+4;
}

int OECDataPacket::getDatalen() {
  return _dataLen;
}

char* OECDataPacket::getData() {
  return _data;
}

char* OECDataPacket::getRaw() {
  return _raw;
}
