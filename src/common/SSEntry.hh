#ifndef _SSENTRY_HH_
#define _SSENTRY_HH_

#include "../inc/include.hh"
#include "../util/RedisUtil.hh"

using namespace std;

class SSEntry {
  private:
    string _filename;
    int _type; //0: online; 1: offline
    int _filesizeMB;

    string _ecidpool; // online: ecid; offline: ecpool
    vector<string> _objList; // OpenEC transfer file into several oecobj, this is the obj name in sequence
    vector<unsigned int> _objLoc;  // location for each obj in this file

    mutex _updateLock; 

  public:
    SSEntry(string filename, int type, int filesizeMB, string ecidpool, vector<string> objname, vector<unsigned int> loc);
    SSEntry(string raw);

    string getFilename();
    int getType();
    int getFilesizeMB();
    string getEcidpool();
    vector<string> getObjlist();
    vector<unsigned int> getObjloc();
    int getIdxOfObj(string objname);
    unsigned int getLocOfObj(string objname);
    string toString();

    // for debug
    void dump();

    // update
    void updateObjLoc(string objname, unsigned int loc);
};

#endif
