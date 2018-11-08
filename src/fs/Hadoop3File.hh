#ifndef _HADOOP3FILE_HH_
#define _HADOOP3FILE_HH_

#include "UnderFile.hh"
#include "../inc/include.hh"
#include "../util/hdfs.h"

class Hadoop3File : public UnderFile {
  public:
    string _objname;
    hdfsFile _objfile; 

    Hadoop3File(string objname, hdfsFile file);
    ~Hadoop3File();
    int getFileSize();
};

#endif

