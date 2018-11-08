#include "OfflineECPool.hh"

OfflineECPool::OfflineECPool(string ecpoolid, ECPolicy* ecpolicy, int basesize) {
  _ecpoolid = ecpoolid;
  _ecpolicy = ecpolicy;
  _basesize = basesize;
}

void OfflineECPool::addObj(string objname, string stripename) {
  _objs.push_back(objname);
  if (find(_stripes.begin(), _stripes.end(), stripename) == _stripes.end())
    _stripes.push_back(stripename);
  unordered_map<string, vector<string>>::iterator it1 = _stripe2objs.find(stripename);
  if (it1 != _stripe2objs.end()) _stripe2objs[stripename].push_back(objname);
  else {
    vector<string> curlist;
    curlist.push_back(objname);
    _stripe2objs.insert(make_pair(stripename, curlist));
  }
  _obj2stripe.insert(make_pair(objname, stripename));
}

int OfflineECPool::getBasesize() {
  return _basesize;
}

string OfflineECPool::getStripeForObj(string objname) {
  string stripename;
  if (find(_objs.begin(), _objs.end(), objname) != _objs.end()) {
    cout << "OfflineECPool::getStripeForObj return " << _obj2stripe[objname] << endl;
    return _obj2stripe[objname];
  } 
  vector<string> objlist;
  if (_stripes.size() == 0) {
    stripename = "oecstripe-"+getTimeStamp();
  } else {
    stripename = _stripes.back();
    if (_stripe2objs[stripename].size() >= _ecpolicy->getK()) {
      stripename = "oecstripe-"+getTimeStamp();
    } 
  }  
  return stripename;
}

vector<string> OfflineECPool::getStripeObjList(string stripename) {
  vector<string> toret;
  unordered_map<string, vector<string>>::iterator it = _stripe2objs.find(stripename);
  if (it != _stripe2objs.end()) toret = _stripe2objs[stripename];
  return toret;
}

string OfflineECPool::getTimeStamp() {
  timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return to_string(ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL);
}

void OfflineECPool::lock() {
  _lockECPool.lock();
}

void OfflineECPool::unlock() {
  _lockECPool.unlock();
}

