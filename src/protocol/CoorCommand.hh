#ifndef _COORCOMMAND_HH_
#define _COORCOMMAND_HH_

#include "../inc/include.hh"
#include "../util/RedisUtil.hh"

using namespace std;

/**
 * CoorCommand Format
 * coor_request: type
 *   type = 0: clientip | filename | ecid | mode |
 *  ? type = 1: clientip | objname |
 *  ? type = 2: clientip | filename | filesize |
 *  ? type = 3: clientip | filename | get redundancyType, filesize, ecid|
 *  ? type = 4: clientip | poolname | stripename |
 *  ? type = 5: clientip | filename | poolname | stripename |
 *  ? type = 6: clientip | filename |   // report lost
 *  ? type = 7: // enable offline encoding 
 *  ? type = 8: clientip | lostobjname |  // stripestore send repair request to coordinator
 *  ? type = 9: // enable repair
 *  ? type = 10: clientip| filename |  // update lostmap in stripestore
 *  ? type = 11: clientip| filename |   // report successfully repair
 */


class CoorCommand {
  private:
    char* _coorCmd;
    int _cmLen;
    string _rKey;
    int _type;
    unsigned int _clientIp;

    // type 0 
    string _filename;
    string _ecid;
    int _mode;
    int _filesizeMB;

  public:
    CoorCommand();
    ~CoorCommand();
    CoorCommand(char* reqStr);

    // basic construction methods
    void writeInt(int value);
    void writeString(string s);
    int readInt();
    string readString();

    int getType();
    unsigned int getClientip();
    string getFilename();
    string getEcid();
    int getMode();
    int getFilesizeMB();

    // send method
    void sendTo(unsigned int ip);
    void sendTo(redisContext* sendCtx);

    // build CoorCommand
    void buildType0(int type,
                    unsigned int ip,
                    string filename, 
                    string ecid,
                    int mode,
                    int filesizeMB);
    // resolve CoorCommand
    void resolveType0();

    // for debug
    void dump();
};

#endif
