#ifndef _OFFLINEECPOOL_HH_
#define _OFFLINEECPOOL_HH_

#include "ECPolicy.hh"
#include "../inc/include.hh"

using namespace std;

class OfflineECPool {
  private:
    string _ecpoolid;
    ECPolicy* _ecpolicy;
    int _basesize;

    mutex _lockECPool;
    vector<string> _objs;
    vector<string> _stripes;
    unordered_map<string, vector<string>> _stripe2objs;
    unordered_map<string, string> _obj2stripe;
    
    string getTimeStamp();

  public:
    OfflineECPool(string ecpoolid, ECPolicy* ecpolicy, int basesize);

    void addObj(string objname, string stripename);

    int getBasesize();
    string getStripeForObj(string objname);
    vector<string> getStripeObjList(string stripename);
    void lock();
    void unlock();
};

#endif
