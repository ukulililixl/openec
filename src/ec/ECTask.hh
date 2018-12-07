#ifndef _ECTASK_HH_
#define _ECTASK_HH_

#include "../inc/include.hh"
#include "../util/RedisUtil.hh"

using namespace std;

/*
 * type 0: Load
 * type 1: Fetch
 * type 2: Compute
 * type 3: Cache
 * type 4: persist
 * type 5: tell (this is a virtual task)
 */
class ECTask {
  private:
    int _type;

    // for type 0
//    int _idx;  // this is idx
    vector<int> _indices;

    // for type 1
    vector<int> _children;

    // for type 2
    // note that children is set for type 1
    // _children
    unordered_map<int, vector<int>> _coefMap;

    // for type 3
    int _persistDSS; // 0: no 1: yes;
    unordered_map<int, int> _refNum;

//    // for type 5
//    int _bind; 

    // serialized task
    char* _taskCmd;
    int _cmLen;
  public:
    ECTask();
    ~ECTask();
    ECTask(char* reqStr);

    void setType(int type);
    void addIdx(int idx);
    void setChildren(vector<int> children);
    void setCoefmap(unordered_map<int, vector<int>> map);
    void setPersistDSS(int pdss);
    void addRef(unordered_map<int, int> ref);
    void addRef(int idx, int ref);
    //void setBind(int id);

    vector<int> getIndices();
    vector<int> getChildren();
    unordered_map<int, vector<int>> getCoefMap();
    int getPersistType();
    unordered_map<int, int> getRefMap();

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
