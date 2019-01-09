#ifndef _FSUTIL_HH_
#define _FSUTIL_HH_

#include "Hadoop20.hh"
#include "Hadoop3.hh"
#include "UnderFS.hh"
#include "../common/Config.hh"
#include "../inc/include.hh"
#include "QFS.hh"

using namespace std;

class FSUtil {
  public:
    static UnderFS* createFS(string type, vector<string> parameter, Config* conf);
    static void deleteFS(string type, UnderFS*);
}; 

#endif
