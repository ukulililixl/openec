#ifndef _FSOBJINPUTSTREAM_HH_
#define _FSOBJINPUTSTREAM_HH_

#include "BlockingQueue.hh"
#include "OECDataPacket.hh"

#include "../fs/UnderFS.hh"

using namespace std;

class FSObjInputStream {
  private:
    Config* _conf;
    string _objname;
    BlockingQueue<OECDataPacket*>* _queue;
    int _dataPktNum;
    bool _exist; 
    int _objbytes;
    int _offset;

    UnderFS* _underfs;
    UnderFile* _underfile;

  public:
    FSObjInputStream(Config* conf, string objname, UnderFS* fs);
    ~FSObjInputStream();
    void readObj();
    void readObj(int slicesize, int unitIdx);
    void readObj(int slicesize);
    void readObj(int w, vector<int> list, int slicesize);
    OECDataPacket* dequeue();
    bool exist();
    bool hasNext();
    int pread(long objoffset, char* buffer, int buflen);
    BlockingQueue<OECDataPacket*>* getQueue();
};

#endif
