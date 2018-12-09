#ifndef _OECWORKER_HH_
#define _OECWORKER_HH_

#include "BlockingQueue.hh"
#include "Config.hh"
#include "FSObjInputStream.hh"
#include "FSObjOutputStream.hh"
#include "OECDataPacket.hh"
//#include "ECBase.hh"
//#include "RSCONV.hh"
//#include "Util/hdfs.h"

#include "../ec/Computation.hh"
#include "../ec/ECTask.hh"
#include "../fs/UnderFS.hh"
#include "../fs/FSUtil.hh"
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

    UnderFS* _underfs;
  public:
    OECWorker(Config* conf);
    ~OECWorker();
    void doProcess();
    // deal with client request
    void clientWrite(AGCommand* agCmd);
    void clientRead(AGCommand* agCmd);
    void onlineWrite(string filename, string ecid, int filesizeMB);
    void offlineWrite(string filename, string ecpoolid, int filesizeMB);
    void readOnline(string filename, int filesizeMB, int ecn, int eck, int ecw);
    void readOffline(string filename, int filesizeMB, int objnum);
    void readOfflineObj(string filename, string objname, int objsizeMB, FSObjInputStream* objstream, int pktnum, int idx);

    // load data from redis
    void loadWorker(BlockingQueue<OECDataPacket*>* readQueue,
                    string keybase,
                    int startid,
                    int step,
                    int round,
                    bool zeropadding);
    // compute
    void computeWorker(vector<ECTask*> compute, 
                       BlockingQueue<OECDataPacket*>** readQueue,
                       FSObjOutputStream** objstreams,
                       int stripenum,
                       int ecn,
                       int eck,
                       int ecw);
    void computeWorker(FSObjInputStream** readStreams,
                              vector<int> idlist,
                              BlockingQueue<OECDataPacket*>* writeQueue,
                              vector<ECTask*> computeTasks,
                              int stripenum,
                              int ecn,
                              int eck,
                              int ecw);
    void computeWorkerDegradedOffline(FSObjInputStream** readStreams,
                                      vector<int> idlist,
                                      unordered_map<int, vector<int>> sid2Cids,
                                      BlockingQueue<OECDataPacket*>* writeQueue,
                                      int lostidx,
                                      vector<ECTask*> computeTasks,
                                      int stripenum,
                                      int ecn,
                                      int eck,
                                      int ecw);

    // deal with coor instruction
    void readDisk(AGCommand* agCmd);
    void fetchCompute(AGCommand* agCmd);
    void persist(AGCommand* agCmd);
    void readFetchCompute(AGCommand* agCmd);

    void selectCacheWorker(BlockingQueue<OECDataPacket*>* cacheQueue,
                           int pktnum,
                           string keybase,
                           int w,
                           vector<int> idxlist,
                           unordered_map<int, int> refs);
    void partialCacheWorker(BlockingQueue<OECDataPacket*>* cacheQueue,
                           int pktnum,
                           string keybase,
                           int w,
                           vector<int> idxlist,
                           unordered_map<int, int> refs);
    void fetchWorker(BlockingQueue<OECDataPacket*>* fetchQueue,
                     string keybase,
                     unsigned int loc,
                     int num);
    void computeWorker(BlockingQueue<OECDataPacket*>** fetchQueue,
                       int nprev,
                       int num,
                       unordered_map<int, vector<int>> coefs,
                       vector<int> cfor,
                       BlockingQueue<OECDataPacket*>** writeQueue,
                       int slicesize);
    void computeWorker(BlockingQueue<OECDataPacket*>** fetchQueue,
                       int nprev,
                       vector<int> prevCids,
                       int num,
                       unordered_map<int, vector<int>> coefs,
                       vector<int> cfor,
		       unordered_map<int, BlockingQueue<OECDataPacket*>*> writeQueue,
                       int slicesize);
    void cacheWorker(BlockingQueue<OECDataPacket*>* writeQueue,
                     string keybase,
                     int num,
                     int refs);
    void cacheWorker(BlockingQueue<OECDataPacket*>* writeQueue,
                     string keybase,
                     int startidx,
                     int num,
                     int refs);
    void cacheWorker(BlockingQueue<OECDataPacket*>* writeQueue,
                     string keybase,
                     int startidx,
                     int step,
                     int num,
                     int refs);

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
