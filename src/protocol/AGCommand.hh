#ifndef _AGCOMMAND_HH_
#define _AGCOMMAND_HH_

#include "../inc/include.hh"
#include "../util/RedisUtil.hh"

using namespace std;

/*
 * OECAgent Command format
 * agent_request: type
 *    type=0 (client write data)| filename | ecid | mode |
 *    type=1 (client read data) | filename |
 *    type=2 (read disk->memory) | read? (| objname | unitIdx | scratio | cid |)
 *    type=3 (fetch->compute->memory) | n prevs | n* (prevloc|prevkey) | m res | m * (n int) | key |
 *   ? type=4 (fetch->disk) | 
 *    type=5 (persis)
 *   ? type=6 (read disk of a list)
 *    type=7 (read disk, fetch remote and compute)
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

    // type 1
    // _filename

    // common variables for ectasks
    bool _shouldSend;
    unsigned int _sendIp = 0;  // if sendIp = 0, then do not send, if sendIp>0, we send it
    string _stripeName; // the stripename for current computation
    int _ecw; // s/c ratio: a pkt is divided into _scratio slices
    int _num; // we based on the conf->pktSize, num = objsize/pktSize;
    unordered_map<int, int> _cacheRefs;

    // type 2
    // read data from disk and write into memory
    string _readObjName;
    vector<int> _readCidList;

    // type 3
    int _nprevs;
    vector<int> _prevCids;
    vector<unsigned int> _prevLocs;
    unordered_map<int, vector<int>> _coefs;

    // type 5
    string _writeObjName;

    // type 7
    // _readObjName
    // _readCidList
    // _nprevs, count readCidList into nprevs
    // _prevLocs
    // _coefs
    // _cacheRefs

    // type 10
    int _ecn;
    int _eck;
    //int _ecw;
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
    char* getCmd();
    int getCmdLen();
    string getFilename();
    string getEcid();
    string getMode();
    int getFilesizeMB();
    bool getShouldSend();
    unsigned int getSendIp();
    string getStripeName();
    int getNum();
    string getReadObjName();
    vector<int> getReadCidList();
    unordered_map<int, int> getCacheRefs();
    int getNprevs();
    vector<int> getPrevCids();
    vector<unsigned int> getPrevLocs();
    unordered_map<int, vector<int>> getCoefs();
    string getWriteObjName();
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
    void buildType1(int type,
                    string filename);
    void buildType2(int type,
                    unsigned int sendIp,
                    string stripeName,
                    int w,
                    int numslices,
                    string readObjName,
                    vector<int> cidlist,
                    unordered_map<int, int> ref);
    void buildType3(int type,
                    unsigned int sendIp,
                    string stripeName,
                    int w,
                    int num,
                    int prevnum,
                    vector<int> prevCids,
                    vector<unsigned int> prevLocs,
                    unordered_map<int, vector<int>> coefs,
                    unordered_map<int, int> ref);
    void buildType5(int type,
                    unsigned int sendIp,
                    string stripename,
                    int w,
                    int num,
                    int prevnum,
                    vector<int> prevCids,
                    vector<unsigned int> prevLocs,
                    string writeobjname);
    void buildType7(int type,
                    unsigned int sendIp,
                    string stripename,
                    int w,
                    int num,
                    string readObjName,
                    vector<int> cidlist,
                    int prevnum,
                    vector<int> prevCids,
                    vector<unsigned int> prevLocs,
                    unordered_map<int, vector<int>> coefs,
                    unordered_map<int, int> ref);
    void buildType10(int type,
                     int ecn,
                     int eck,
                     int ecw,
                     int computen);
    void buildType11(int type,
                     int objnum,
                     int basesizeMB);
    // resolve AGCommand
    void resolveType0();
    void resolveType1();
    void resolveType2();
    void resolveType3();
    void resolveType5();
    void resolveType7();
    void resolveType10();
    void resolveType11();

    // for debug
    void dump();
};

#endif
