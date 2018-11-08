#ifndef _UNDERFS_HH_
#define _UNDERFS_HH_

#include "UnderFile.hh"
#include "../common/Config.hh"
#include "../inc/include.hh"

using namespace std;

class UnderFS {
  public:
    Config* _conf;

    UnderFS();
    UnderFS(vector<string> param, Config* conf);    
    ~UnderFS();

    virtual UnderFile* openFile(string filename, string mode) = 0;
    virtual void writeFile(UnderFile* file, char* buffer, int len) = 0;
    virtual void flushFile(UnderFile* file) = 0;
    virtual void closeFile(UnderFile* file) = 0;
    virtual int readFile(UnderFile* file, char* buffer, int len) = 0;
    virtual int pReadFile(UnderFile* file, int offset, char* buffer, int len) = 0;
    virtual int getFileSize(UnderFile* file) = 0;
};

#endif
