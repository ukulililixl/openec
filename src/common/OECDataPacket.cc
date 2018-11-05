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

int OECDataPacket::getDatalen() {
  return _dataLen;
}
