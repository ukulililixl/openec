#ifndef _STRIPESTORE_HH_
#define _STRIPESTORE_HH_

#include "BlockingQueue.hh"
#include "Config.hh"
#include "SSEntry.hh"
//#include "CoorCommand.hh"
//#include "ECPolicy.hh"
//#include "OfflineECPool.hh"

#include "../inc/include.hh"
#include "../ec/OfflineECPool.hh"

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
//    BlockingQueue<pair<string, string>> _pendingECQueue;
//    mutex _lockPECQueue;
//    unordered_map<string, int> _lostMap;
//    mutex _lockLostMap;
//    vector<string> _ECInProgress;
//    mutex _lockECInProgress;
//    vector<string> _RPInProgress;
//    mutex _lockRPInProgress;
//
//    mutex _lockRandom;
//
//    bool _enableScan;
//    struct timeval _startEnc, _endEnc; 
//
//    bool _enableRepair;
  public:
    StripeStore(Config* conf);

    bool existEntry(string filename);
    void insertEntry(SSEntry* entry);
    SSEntry* getEntry(string filename);
    SSEntry* getEntryFromObj(string objname);

    OfflineECPool* getECPool(string ecpoolid, ECPolicy* ecpolicy, int basesize);

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
//    OfflineECPool* getECPool(string poolname, ECPolicy* ecpolicy);
//    OfflineECPool* getECPool(string poolname);
//    void addECPool(OfflineECPool* ecpool);
//    void addToECQueue(string poolname, string stripename);
//    int getRandomInt(int size);
//    
//    // offline encode
    void scanning();
//    void setScan(bool status);
//    void startECStripe(string stripename);
//    void finishECStripe(string stripename);
//    int getECInProgressNum();
//    
    // repair
    void scanRepair();
//    void addToLostMap(string objname);
//    void setRepair(bool status);
//    void startRepair(string objname);
//    void finishRepair(string objname);
//    int getRPInProgressNum();
  
};

#endif
