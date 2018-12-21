#ifndef _COORCOMMAND_HH_
#define _COORCOMMAND_HH_

#include "../inc/include.hh"
#include "../util/RedisUtil.hh"

using namespace std;

/**
 * CoorCommand Format
 * coor_request: type
 *   type = 0: clientip | filename | ecid | mode | filesizeMB |
 *   type = 1: clientip | objname |
 *   type = 2: clientip | filename |
 *   type = 3: clientip | filename | get redundancyType, filesize, ecid|
 *   type = 4: clientip | poolname | stripename |
 *   type = 5: clientip | objname // offline degraded for object
 *   type = 5: clientip | filename | poolname | stripename |
 *   type = 6: clientip | filename |   // report lost
 *   type = 7:  0 (disable)/ 1 (enable) | encode/repair
 *   type = 8: clientip | lostobjname |  // stripestore send repair request to coordinator
 *   type = 9: clientip | filename | corrupnum | idx1-idx2..| // 
 *  ? type = 10: clientip| filename |  // update lostmap in stripestore
 *   type = 11: clientip| filename |   // report successfully repair
 *   type = 12: clientip | benchname | 
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

    // type 1
    int _numOfReplicas;

    // type 4
    // client ip
    string _ecpoolid;
    string _stripename;

    // type 5
    // _filename

    // type 6
    // _filename

    // type 7
    int _op; // enable/disable
    string _ectype; // encode/repair

    // type9
    // _filename
    vector<int> _corruptIdx;

    // type12
    string _benchname;

  public:
    CoorCommand();
    ~CoorCommand();
    CoorCommand(char* reqStr);

    // basic construction methods
    void writeInt(int value);
    void writeString(string s);
    int readInt();
    int readRawInt();
    string readString();

    int getType();
    unsigned int getClientip();
    string getFilename();
    string getEcid();
    int getMode();
    int getFilesizeMB();
    int getNumOfReplicas();
    string getECPoolId();
    string getStripeName();
    int getOp();
    string getECType();
    vector<int> getCorruptIdx();
    string getBenchName();

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
    void buildType2(int type,
                    unsigned int ip,
                    string filename);
    void buildType3(int type,
                    unsigned int ip,
                    string filename);
    void buildType4(int type,
                    unsigned int ip,
                    string poolname,
                    string stripename);
    void buildType5(int type,
                    unsigned int ip,
                    string objname);
    void buildType7(int type,
                    int op,
                    string ectype);
    void buildType8(int type,
                    unsigned int ip,
                    string objname);
    void buildType9(int type, 
                    unsigned int ip,
                    string filename,
                    vector<int> corruptIdx);
    void buildType12(int type,
                     unsigned int ip,
                     string benchname);
    // resolve CoorCommand
    void resolveType0();
    void resolveType1();
    void resolveType2();
    void resolveType3();
    void resolveType4();
    void resolveType5();
    void resolveType6();
    void resolveType7();
    void resolveType8();
    void resolveType9();
    void resolveType11();
    void resolveType12();

    // for debug
    void dump();
};

#endif
