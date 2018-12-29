#ifndef _QFS_HH_
#define _QFS_HH_

#include "QFSFile.hh"
#include "UnderFS.hh"
#include "UnderFile.hh"
#include "../common/Config.hh"
#include "../inc/include.hh"
#include "../util/KfsClient.h"
#include "../util/KfsAttr.h"

using namespace std;

class QFS : public UnderFS {
  private:
    string _ip;
    int _port;
    KFS::KfsClient* _fs;
  public:
    QFS(vector<string> params, Config* conf);
    ~QFS();
    QFSFile* openFile(string filename, string mode);
    void writeFile(UnderFile* file, char* buffer, int len);
    void flushFile(UnderFile* file);
    void closeFile(UnderFile* file);
    int readFile(UnderFile* file, char* buffer, int len);
    int pReadFile(UnderFile* file, int offset, char* buffer, int len);
    int getFileSize(UnderFile* file);
};

#endif
