#ifndef _ECTASK_HH_
#define _ECTASK_HH_

#include "../inc/include.hh"
#include "../util/RedisUtil.hh"

using namespace std;

/*
 * type 0: Load
 * type 1: Fetch
 * type 2: Compute
 * type 3: Persist
 */
class ECTask {
  private:
    int _type;

    // for type 0
    int _idx;  // this is idx

    // for type 1
    vector<int> _children;

    // for type 2
    // note that children is set for type 1
    // _children
    unordered_map<int, vector<int>> _coefMap;

    // for type 3
    unordered_map<int, int> _refNum;

    // serialized task
    char* _taskCmd;
    int _cmLen;
  public:
    ECTask();
    ~ECTask();
    ECTask(char* reqStr);

    void setType(int type);
    void setChildren(vector<int> children);
    void setCoefmap(unordered_map<int, vector<int>> map);

    // basic construction methods
    void writeInt(int value);
    void writeString(string s);
    int readInt();
    string readString();

    void buildType2();
    void resolveType2();

    void sendTo(string key, unsigned int ip);

    // for debug
    void dump();
};

#endif
