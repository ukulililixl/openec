#ifndef _STRIPESTORE_HH_
#define _STRIPESTORE_HH_

#include "BlockingQueue.hh"
#include "Config.hh"
#include "SSEntry.hh"
//#include "ECPolicy.hh"
//#include "OfflineECPool.hh"

#include "../inc/include.hh"
#include "../ec/OfflineECPool.hh"
#include "../protocol/CoorCommand.hh"

using namespace std;

class StripeStore {
  private:
    Config* _conf;

    // map original file name to SSEntry
    // for online-encoded file, we can get objname for each split
    // for offline encoded file, we can get splited blocks
    unordered_map<string, SSEntry*> _ssEntryMap;  
    mutex _lockSSEntryMap;
    // map objname to original file name
    // for online encoded file, given a split name, we can get the original filename
    // for offline encoded file, given a block name, we can get the original filename
    unordered_map<string, SSEntry*> _objEntryMap;
    mutex _lockObjEntryMap;
    
    unordered_map<unsigned int, int> _dataLoadMap;
    mutex _lockDLMap;
    unordered_map<unsigned int, int> _controlLoadMap;
    mutex _lockCLMap;
    unordered_map<unsigned int, int> _repairLoadMap;
    mutex _lockRLMap;
    unordered_map<unsigned int, int> _encodeLoadMap;
    mutex _lockELMap;

    unordered_map<string, OfflineECPool*> _offlineECPoolMap;
    mutex _lockECPoolMap;
    BlockingQueue<pair<string, string>> _pendingECQueue;
    mutex _lockPECQueue;
    vector<string> _ECInProgress;
    mutex _lockECInProgress;
    unordered_map<string, int> _lostMap;
    mutex _lockLostMap;
    vector<string> _RPInProgress;
    mutex _lockRPInProgress;

    mutex _lockRandom;

    // offline encoding
    bool _enableScan;
    struct timeval _startEnc, _endEnc; 

    bool _enableRepair;

    // backup
    string _entryStorePath = "entryStore";
    ofstream _entryStore;
    mutex _lockEntryStore;

    string _poolStorePath = "poolStore";
    ofstream _poolStore;
    mutex _lockPoolStore;
    
  public:
    StripeStore(Config* conf);

    bool existEntry(string filename);
    void insertEntry(SSEntry* entry);
    SSEntry* getEntry(string filename);
    SSEntry* getEntryFromObj(string objname);

    OfflineECPool* getECPool(string ecpoolid, ECPolicy* ecpolicy, int basesize);
    OfflineECPool* getECPool(string ecpoolid);
    void insertECPool(string ecpoolid, OfflineECPool* pool);

//    int getSize();
    void increaseDataLoadMap(unsigned int ip, int load);
    void increaseControlLoadMap(unsigned int ip, int load);
    void increaseRepairLoadMap(unsigned int ip, int load);
    void increaseEncodeLoadMap(unsigned int ip, int load);
    int getDataLoad(unsigned int ip);
    int getControlLoad(unsigned int ip);
    int getRepairLoad(unsigned int ip);
    int getEncodeLoad(unsigned int ip);

//    bool poolExists(string poolname);
//    void addECPool(OfflineECPool* ecpool);
//    void addToECQueue(string poolname, string stripename);
//    int getRandomInt(int size);
//    

    // set status
    void setECStatus(int op, string ectype);

//    // offline encode
    void scanning();
    void addEncodeCandidate(string ecpoolid, string stripename);
    int getECInProgressNum();
    void startECStripe(string stripename);
//    void setScan(bool status);
    void finishECStripe(OfflineECPool* ecpool, string stripename);
    
    // repair
    void scanRepair();
    void addLostObj(string objname);
//    void setRepair(bool status);
    void startRepair(string objname);
    void finishRepair(string objname);
    int getRPInProgressNum();
  
    // backup
    void backupEntry(string entrystr);
    void backupPoolStripe(string stripename);
};

#endif
