#ifndef _HADOOP20FILE_HH_
#define _HADOOP20FILE_HH_

#include "UnderFile.hh"
#include "../inc/include.hh"
#include "../util/hdfs.h"

class Hadoop20File : public UnderFile {
  public:
    string _objname;
    hdfsFile _objfile;

    Hadoop20File(string objname, hdfsFile file);
};

#endif

