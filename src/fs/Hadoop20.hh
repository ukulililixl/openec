#ifndef _HADOOP20_HH_
#define _HADOOP20_HH_

#include "Hadoop20File.hh"
#include "UnderFS.hh"
#include "UnderFile.hh"
#include "../common/Config.hh"
#include "../inc/include.hh"
#include "../util/hdfs.h"

using namespace std;

class Hadoop20 : public UnderFS {
  public:
    string _ip;
    int _port;
    hdfsFS _fs;

    Hadoop20(vector<string> params, Config* conf);
    ~Hadoop20();
    Hadoop20File* openFile(string filename, string mode);
    void writeFile(UnderFile* file, char* buffer, int len);
    void flushFile(UnderFile* file);
    void closeFile(UnderFile* file);
    int readFile(UnderFile* file, char* buffer, int len);
    int pReadFile(UnderFile* file, int offset, char* buffer, int len);
    int getFileSize(UnderFile* file);
};

#endif
