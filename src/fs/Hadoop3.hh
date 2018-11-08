#ifndef _HADOOP3_HH_
#define _HADOOP3_HH_

#include "Hadoop3File.hh"
#include "UnderFS.hh"
#include "UnderFile.hh"
#include "../common/Config.hh"
#include "../inc/include.hh"
#include "../util/hdfs.h"

using namespace std;

class Hadoop3 : public UnderFS {
  private:
    string _ip;
    int _port;
    hdfsFS _fs;
  public:
    Hadoop3(vector<string> params, Config* conf);
    ~Hadoop3();
    Hadoop3File* openFile(string filename, string mode);
    void writeFile(UnderFile* file, char* buffer, int len);
    void flushFile(UnderFile* file);
    void closeFile(UnderFile* file);
    int readFile(UnderFile* file, char* buffer, int len);
    int pReadFile(UnderFile* file, int offset, char* buffer, int len);
    int getFileSize(UnderFile* file);
};

#endif
