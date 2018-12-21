#ifndef _COORDINATOR_HH_
#define _COORDINATOR_HH_

//#include "AGCommand.hh"
#include "Config.hh"
#include "FSObjInputStream.hh"
//#include "RedisUtil.hh"
#include "StripeStore.hh"
//#include "SSEntry.hh"
//#include "UnderFile.hh"
//#include "Util/hdfs.h"

#include "../ec/ECDAG.hh"
#include "../ec/OfflineECPool.hh"
#include "../fs/FSUtil.hh"
#include "../fs/UnderFS.hh"
#include "../inc/include.hh"
#include "../protocol/AGCommand.hh"
#include "../protocol/CoorCommand.hh"

using namespace std;

class Coordinator {
  private:
    Config* _conf;
    redisContext* _localCtx;
    StripeStore* _stripeStore;
    UnderFS* _underfs;

  public:
    Coordinator(Config* conf, StripeStore* ss);
    ~Coordinator();

    void doProcess();

    void registerFile(CoorCommand* coorCmd);
    void getLocation(CoorCommand* coorCmd);
    void finalizeFile(CoorCommand* coorCmd);
    void offlineEnc(CoorCommand* coorCmd);
    void setECStatus(CoorCommand* coorCmd);
    void getFileMeta(CoorCommand* coorCmd);
    void reportLost(CoorCommand* coorCmd);
    void offlineDegradedInst(CoorCommand* coorCmd);
    void onlineDegradedInst(CoorCommand* coorCmd);
    void repairReqFromSS(CoorCommand* coorCmd);
    void reportRepaired(CoorCommand* coorCmd);
    void coorBenchmark(CoorCommand* coorCmd);

    void registerOnlineEC(unsigned int clientIp, string filename, string ecid, int filesizeMB);
    void registerOfflineEC(unsigned int clientIp, string filename, string ecpoolid, int filesizeMB);
    vector<unsigned int> getCandidates(vector<unsigned int> placedIp, vector<int> placedIdx, vector<int> colocWith);
    unsigned int chooseFromCandidates(vector<unsigned int> candidates, string policy, string type); // policy:random/balance; type:control/data/other
//    void onlineECInst(string filename, SSEntry* ssentry, unsigned int ip);
//    void offlineECInst(string filename, SSEntry* ssentry, unsigned int ip);
    void nonOptOfflineDegrade(string lostobj, unsigned int clientIp, OfflineECPool* ecpool, ECPolicy* ecpolicy);
    void optOfflineDegrade(string lostobj, unsigned int clientIp, OfflineECPool* ecpool, ECPolicy* ecpolicy);
    void recoveryOnline(string filename);
    void recoveryOffline(string filename);
};

#endif
