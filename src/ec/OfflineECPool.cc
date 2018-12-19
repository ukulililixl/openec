#include "OfflineECPool.hh"

OfflineECPool::OfflineECPool(string ecpoolid, ECPolicy* ecpolicy, int basesize) {
  _ecpoolid = ecpoolid;
  _ecpolicy = ecpolicy;
  _basesize = basesize;
}

void OfflineECPool::addObj(string objname, string stripename) {
  _objs.insert(make_pair(objname, false));
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

void OfflineECPool::finalizeObj(string objname) {
  assert(_objs.find(objname) != _objs.end());
  _objs[objname] = true;
}

bool OfflineECPool::isCandidateForEC(string stripename) {
  int eck = _ecpolicy->getK();
  vector<string> objlist = _stripe2objs[stripename];
  if (objlist.size() < eck) return false;
  if (objlist.size() == eck) {
    // this might be a candidate, check finalize for each obj
    bool toret = true;
    for (auto obj: objlist) {
      if (!_objs[obj]) {
        toret = false;
        break;
      }
    }
    return toret;
  } else {
    // this stripe has been erasure-coded
    return false;
  }
}

int OfflineECPool::getBasesize() {
  return _basesize;
}

string OfflineECPool::getStripeForObj(string objname) {
  string stripename;
//  if (find(_objs.begin(), _objs.end(), objname) != _objs.end()) {
//    cout << "OfflineECPool::getStripeForObj return " << _obj2stripe[objname] << endl;
//    return _obj2stripe[objname];
//  } 
  if (_objs.find(objname) != _objs.end()) {
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

ECPolicy* OfflineECPool::getEcpolicy() {
  return _ecpolicy;
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

string OfflineECPool::stripe2String(string stripename) {
  string toret = "";
  toret += _ecpoolid+";";
  toret += stripename+";";
  vector<string> objlist = _stripe2objs[stripename];
  for (int i=0; i<objlist.size(); i++) {
    toret += objlist[i]+";";
  }

  return toret;
}

void OfflineECPool::constructPool(vector<string> items) {
  // we skip items[0], which is poolid
  // only used when stripestore initialize
  _lockECPool.lock();
  string stripename = items[1];
  cout << stripename << endl;
  _stripes.push_back(stripename);
  int objnum = (items.size() - 2);
  vector<string> stripelist;
  for (int i=0; i<objnum; i++) {
    int idx = 2+i;
    string objname = items[idx];
    _objs.insert(make_pair(objname, true));
    _obj2stripe.insert(make_pair(objname, stripename));
    stripelist.push_back(objname);
    cout << objname << endl;
  }
  _stripe2objs.insert(make_pair(stripename, stripelist));
  _lockECPool.unlock();
}
