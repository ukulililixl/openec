#ifndef _FSUTIL_HH_
#define _FSUTIL_HH_

#ifdef HDFSRAID
#include "Hadoop20.hh"
#endif

#ifdef HDFS3
#include "Hadoop3.hh"
#endif

#ifdef QFS
#include "QuantcastFS.hh"
#endif

#include "UnderFS.hh"
#include "../common/Config.hh"
#include "../inc/include.hh"

using namespace std;

class FSUtil {
  public:
    static UnderFS* createFS(string type, vector<string> parameter, Config* conf);
    static void deleteFS(string type, UnderFS*);
}; 

#endif
