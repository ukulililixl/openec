#ifndef _OECINPUTSTREAM_HH_
#define _OECINPUTSTREAM_HH_

#include "BlockingQueue.hh"
#include "Config.hh"
#include "OECDataPacket.hh"

#include "../inc/include.hh"
#include "../protocol/AGCommand.hh"
#include "../util/RedisUtil.hh"

using namespace std;

class OECInputStream {
  private:
    Config* _conf;
    string _filename;
    redisContext* _localCtx;

    BlockingQueue<OECDataPacket*>* _readQueue;
    int _filesizeMB;
    thread _collectThread;
  public:
    OECInputStream(Config* conf, 
                   string filename);
    ~OECInputStream();
    void init();
    void readWorker(BlockingQueue<OECDataPacket*>* readQueue,
                   string keybase);
    void output2file(string saveas);
    void close();
};

#endif
