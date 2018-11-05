#ifndef _OECWORKER_HH_
#define _OECWORKER_HH_

#include "BlockingQueue.hh"
#include "Config.hh"
#include "OECDataPacket.hh"
//#include "ECBase.hh"
//#include "FSObjInputStream.hh"
//#include "FSObjOutputStream.hh"
//#include "FSUtil.hh"
//#include "RSCONV.hh"
//#include "UnderFS.hh"
//#include "Util/hdfs.h"

#include "../ec/ECTask.hh"
#include "../inc/include.hh"
#include "../protocol/AGCommand.hh"
#include "../protocol/CoorCommand.hh"
#include "../util/RedisUtil.hh"

class OECWorker {
  private: 
    Config* _conf;

    redisContext* _processCtx;
    redisContext* _localCtx;
    redisContext* _coorCtx;

//    UnderFS* _underfs;
  public:
    OECWorker(Config* conf);
    ~OECWorker();
    void doProcess();
    // deal with client request
    void clientWrite(AGCommand* agCmd);
    void onlineWrite(string filename, string ecid, int filesizeMB);

    void loadWorker(BlockingQueue<OECDataPacket*>* readQueue,
                    string keybase,
                    int startid,
                    int step,
                    int round,
                    bool zeropadding);
//    void offlineWrite(AGCommand* agCmd);
//    void clientRead(AGCommand* agCmd);
//    void readDisk(AGCommand* agCmd);
//    void readDiskList(AGCommand* agCmd);
//    void fetchCompute(AGCommand* agCmd);
//    void readFetchCompute(AGCommand* agCmd);
//    void persist(AGCommand* agCmd);
//    void clientReadOffline(string filename,
//                           int filesizeMB,
//                           string poolname,
//                           string stripename);
//
//    // for online framework
//    void readWorker(BlockingQueue<OECDataPacket*>* readQueue,
//                    string keybase,
//                    int num);
//    void readWorker(BlockingQueue<OECDataPacket*>* fetchQueue,
//                    string keybase,
//                    int startid,
//                    int step,
//                    int max);
//    void encWorker(BlockingQueue<OECDataPacket*>* readQueue,
//                   FSObjOutputStream** objstreams,
//                   ECPolicy* ecpolicy,
//                   string mode);
//    void encWorker(BlockingQueue<OECDataPacket*>** fetchQueue,
//                   FSObjOutputStream** objstreams,
//                   ECPolicy* ecpolicy,
//                   string mode,
//                   int stripenum);
//    void writeWorker(BlockingQueue<OECDataPacket*>* writeQueue,
//                     string keybase);
//    void writeWorker2(BlockingQueue<OECDataPacket*>* writeQueue,
//                     string keybase);
//    void writeWorker(BlockingQueue<OECDataPacket*>* writeQueue,
//                     string keybase,
//                     int startid,
//                     int step,
//                     int max);
//    void selectWriteWorker(BlockingQueue<OECDataPacket*>* writeQueue,
//                           int pktnum,
//                           string keybase,
//                           int scratio,
//                           vector<int> idxlist);
//    void decWorker(FSObjInputStream** readStreams,
//                   vector<int> idlist,
//                   bool needRecovery,
//                   BlockingQueue<OECDataPacket*>* writeQueue,
//                   //BlockingQueue<OECDataPacket*>** clientWriteQueue,
//                   ECBase* ec,
//                   vector<int> avail,
//                   vector<int> torec);
//
//    // for offline framework
//    void readWorker(BlockingQueue<OECDataPacket*>* readQueue,
//                    string objname,
//                    int num,
//                    int pktsize,
//                    int slicesize,
//                    int unitidx);
//    void writeWorker(BlockingQueue<OECDataPacket*>* writeQueue,
//                     string keybase,
//                     int num);
//    void fetchWorker(BlockingQueue<OECDataPacket*>* fetchQueue,
//                     string keybase,
//                     unsigned int loc,
//                     int num);
//    void computeWorker(BlockingQueue<OECDataPacket*>** fetchQueue,
//                       int nprev,
//                       int num,
//                       unordered_map<int, vector<int>> coefs,
//                       vector<int> cfor,
//                       BlockingQueue<OECDataPacket*>** writeQueue,
//                       int slicesize);
//
//    // for test
//    void testWriteWorker(BlockingQueue<OECDataPacket*>* writeQueue,
//                            string keybase,
//                            int num);
};

#endif
