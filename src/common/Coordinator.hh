#ifndef _COORDINATOR_HH_
#define _COORDINATOR_HH_

//#include "AGCommand.hh"
#include "Config.hh"
//#include "CoorCommand.hh"
//#include "FSObjInputStream.hh"
//#include "FSUtil.hh"
//#include "OfflineECPool.hh"
//#include "RedisUtil.hh"
#include "StripeStore.hh"
//#include "SSEntry.hh"
//#include "UnderFile.hh"
//#include "UnderFS.hh"
//#include "Util/hdfs.h"

#include "../inc/include.hh"

using namespace std;

class Coordinator {
  private:
    Config* _conf;
//    redisContext* _localCtx;
//    StripeStore* _stripeStore;
//    UnderFS* _underfs;

  public:
    Coordinator(Config* conf, StripeStore* ss);
    ~Coordinator();

    void doProcess();
//
//    void updateMeta(CoorCommand* coorCmd);
//    void getLocation(CoorCommand* coorCmd);
//    void updateFileSize(CoorCommand* coorCmd);
//    void getFileMeta(CoorCommand* coorCmd);
//    void offlineDegraded(CoorCommand* coorCmd);
//    void onlineDegradedUpdate(CoorCommand* coorCmd);
//    void reportLost(CoorCommand* coorCmd);
//    void enableEncoding(CoorCommand* coorCmd);
//    void repairReqFromSS(CoorCommand* coorCmd);
//    void enableRepair(CoorCommand* coorCmd);
//    void reportRepaired(CoorCommand* coorCmd);
//
//    void offlineEnc(CoorCommand* coorCmd);
//    void recoveryOnline(string filename);
//    void recoveryOffline(string filename);
//    vector<unsigned int> getCandidates(vector<unsigned int> ips,
//                                       vector<int> colocWith,
//                                       vector<int> notColocWith);
//    vector<unsigned int> getCandidates(vector<unsigned int> avoid);
//    unsigned int chooseFromCandidates(vector<unsigned int> candidates);
//    unsigned int chooseFromCandidates(vector<unsigned int> candidates, string policy, string type); // policy:random/balance; type:control/data/other

};

#endif
