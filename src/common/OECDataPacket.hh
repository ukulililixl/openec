#ifndef _OECDATAPACKET_HH_
#define _OECDATAPACKET_HH_

#include "../inc/include.hh"

using namespace std;

class OECDataPacket {
  private:
    int _dataLen;
    char* _raw;  // the first 4 bytes are _dataLen in network bytes order, follows the data content
                 // so the length of _raw is 4+_dataLen
    char* _data;

  public:
    OECDataPacket();
    OECDataPacket(char* raw);
    OECDataPacket(int len);
    ~OECDataPacket();
    void setRaw(char* raw);

    int getDatalen();
    char* getData();
    char* getRaw();
};

#endif
