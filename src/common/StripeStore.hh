#ifndef _STRIPESTORE_HH_
#define _STRIPESTORE_HH_

#include "BlockingQueue.hh"
#include "Config.hh"
//#include "CoorCommand.hh"
//#include "ECPolicy.hh"
//#include "OfflineECPool.hh"
//#include "SSEntry.hh"

#include "../inc/include.hh"

using namespace std;

class StripeStore {
  private:
    Config* _conf;

//    unordered_map<string, SSEntry*> _ssEntryMap;
//    mutex _lockSSEntryMap;
//    unordered_map<unsigned int, int> _dataLoadMap;
//    mutex _lockDLMap;
//    unordered_map<unsigned int, int> _controlLoadMap;
//    mutex _lockCLMap;
//    unordered_map<unsigned int, int> _repairLoadMap;
//    mutex _lockRLMap;
//    unordered_map<unsigned int, int> _encodeLoadMap;
//    mutex _lockELMap;
//    unordered_map<string, OfflineECPool*> _offlineECPoolMap;
//    mutex _lockECPoolMap;
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

//    void insertEntry(SSEntry* entry);
//    bool existEntry(string filename);
//    SSEntry* getEntry(string filename);
//
//    int getSize();
//    void increaseDataLoadMap(unsigned int ip, int load);
//    void increaseControlLoadMap(unsigned int ip, int load);
//    void increaseRepairLoadMap(unsigned int ip, int load);
//    void increaseEncodeLoadMap(unsigned int ip, int load);
//    int getDataLoad(unsigned int ip);
//    int getControlLoad(unsigned int ip);
//    int getRepairLoad(unsigned int ip);
//    int getEncodeLoad(unsigned int ip);
//
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
