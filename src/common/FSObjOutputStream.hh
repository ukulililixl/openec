#ifndef _FSOBJOUTPUTSTREAM_HH_
#define _FSOBJOUTPUTSTREAM_HH_

#include "BlockingQueue.hh"
#include "../common/Config.hh"
#include "../common/OECDataPacket.hh"
#include "../fs/UnderFS.hh"
#include "../inc/include.hh"

using namespace std;

class FSObjOutputStream {
  private:
    Config* _conf;
    string _objname;
    BlockingQueue<OECDataPacket*>* _queue;
    int _totalPktNum;
    int _dataPktNum;
    bool _finish;
    int _objsize;

    // fsinterface
    UnderFS* _underfs;
    UnderFile* _underfile;

  public:
    FSObjOutputStream(Config* conf, string objname, UnderFS* fs, int pktnum);
    ~FSObjOutputStream();
    void writeObj();
    void enqueue(OECDataPacket* pkt);
    bool getFinish();
    BlockingQueue<OECDataPacket*>* getQueue(); 
};

#endif
