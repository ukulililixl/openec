#ifndef COMMON_COORCOOMAND_H
#define COMMON_COORCOMMAND_H

#include <iostream>
#include <string>

#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <hiredis/hiredis.h>

namespace KFS {
using std::string;

class CoorCommand {
  private:
    char* _coorCmd;
    int _cmLen;
    string _rKey;

    int _type;
    unsigned int _clientIp;
    string _fileName;
    int _numOfReplicas;
  public:
    CoorCommand();
    ~CoorCommand();

    // basic construction methods
    void writeInt(int value);
    void writeRawInt(int value);
    void writeString(string s);
    int readInt();
    int readRawInt();
    string readString();

    void sendTo(unsigned int);

    redisContext* createContext(string ip, int port);
    string ip2Str(unsigned int ip);

    void buildType1(int type, unsigned int clientip, string filename, int replica);
    void buildType6(int type, unsigned int clientip, string objname);
    void buildType11(int type, unsigned int clientip, string objname);
    string waitForLocation(unsigned int coorip, string objname);
};
}
#endif
