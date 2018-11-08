#ifndef _AGCOMMAND_HH_
#define _AGCOMMAND_HH_

#include "../inc/include.hh"
#include "../util/RedisUtil.hh"

using namespace std;

/*
 * OECAgent Command format
 * agent_request: type
 *    type=0 (client write data)| filename | ecid | mode |
 *   ? type=1 (client read data) | filename |
 *   ? type=2 (read disk->memory) | read? (| objname | unitIdx | scratio | cid |)
 *   ? type=3 (fetch->compute->memory) | n prevs | n* (prevloc|prevkey) | m res | m * (n int) | key |
 *   ? type=4 (fetch->disk) | 
 *   ? type=5 (persis)
 *   ? type=6 (read disk of a list)
 *   ? type=7 (read disk, fetch remote and compute)
 *    type=10: (coor return cmd summary for client to online encoding)| |
 *    type=11: (coor return cmd summary for client to write obj of offline encoding)
 */


class AGCommand {
  private:
    char* _agCmd = 0;
    int _cmLen = 0;

    string _rKey;

    int _type;

    // type 0
    string _filename;
    string _ecid;  // for writing with online encoding, it refers to ecid. Other wise, it refers to ecpoolid
    string _mode;
    int _filesizeMB;

    // type 10
    int _ecn;
    int _eck;
    int _ecw;
    int _computen;

    // type 11
    int _objnum;
    int _basesizeMB;
    
  public:
    AGCommand();
    ~AGCommand();
    AGCommand(char* reqStr);

    // basic construction methods
    void writeInt(int value);
    void writeString(string s);
    int readInt();
    string readString();

    int getType();
    string getFilename();
    string getEcid();
    string getMode();
    int getFilesizeMB();
    int getN();
    int getK();
    int getW();
    int getComputen();
    int getObjnum();
    int getBasesizeMB();

    // send method
    void setRkey(string key);
    void sendTo(unsigned int ip);

    // build AGCommand
    void buildType0(int type,
                    string filename,
                    string ecid,
                    string mode,
                    int filesizeMB);
    void buildType10(int type,
                     int ecn,
                     int eck,
                     int ecw,
                     int computen);
    void buildType11(int type,
                     int objnum,
                     int basesize);
    // resolve AGCommand
    void resolveType0();
    void resolveType10();
    void resolveType11();

    // for debug
    void dump();
};

#endif
