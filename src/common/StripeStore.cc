#include "StripeStore.hh"

StripeStore::StripeStore(Config* conf) {
  _conf = conf;
//   if (_conf->_encode_scheduling == "delay") _enableScan = false;
//   else _enableScan = true;
// 
//   if (_conf->_repair_scheduling == "delay") _enableRepair = false;
//   else if (_conf->_repair_scheduling == "threshold") _enableRepair = false;
//   else _enableRepair = true;
}

// void StripeStore::insertEntry(SSEntry* entry) {
//   unordered_map<string, SSEntry*>::iterator it = _ssEntryMap.find(entry->_filename);
//   if (it == _ssEntryMap.end()) {
//     // the entry does not exist, insert this entry
//     _lockSSEntryMap.lock();
//     _ssEntryMap.insert(make_pair(entry->_filename, entry));
//     _lockSSEntryMap.unlock();
//   } else {
//     // the entry exist, only need to update the entry
//     _lockSSEntryMap.lock();
//     SSEntry* oldEntry = _ssEntryMap[entry->_filename];
//     _ssEntryMap[entry->_filename] = entry;
//     _lockSSEntryMap.unlock();
//     // need to delete the old entry?
//     delete oldEntry;
//   }
// }
// 
// bool StripeStore::existEntry(string filename) {
//   unordered_map<string, SSEntry*>::iterator it = _ssEntryMap.find(filename);
//   return it == _ssEntryMap.end() ? false:true;
// }
// 
// SSEntry* StripeStore::getEntry(string filename) {
//   if (existEntry(filename)) return _ssEntryMap[filename];
//   else return NULL;
// }
// 
// int StripeStore::getSize() {
//   return _ssEntryMap.size();
// }
// 
// void StripeStore::increaseDataLoadMap(unsigned int ip, int load) {
//   _lockDLMap.lock();
//   unordered_map<unsigned int, int>::iterator it = _dataLoadMap.find(ip);
//   if (it == _dataLoadMap.end()) _dataLoadMap.insert(make_pair(ip, load));
//   else _dataLoadMap[ip] += load;
//   _lockDLMap.unlock();
// }
// 
// void StripeStore::increaseControlLoadMap(unsigned int ip, int load) {
//   _lockCLMap.lock();
//   unordered_map<unsigned int, int>::iterator it = _controlLoadMap.find(ip);
//   if (it == _controlLoadMap.end()) _controlLoadMap.insert(make_pair(ip, load));
//   else _controlLoadMap[ip] += load;
//   _lockCLMap.unlock();
// }
// 
// void StripeStore::increaseRepairLoadMap(unsigned int ip, int load) {
//   _lockRLMap.lock();
//   unordered_map<unsigned int, int>::iterator it = _repairLoadMap.find(ip);
//   if (it == _repairLoadMap.end()) _repairLoadMap.insert(make_pair(ip, load));
//   else _repairLoadMap[ip] += load;
//   _lockRLMap.unlock();
// }
// 
// void StripeStore::increaseEncodeLoadMap(unsigned int ip, int load) {
//   _lockELMap.lock();
//   unordered_map<unsigned int, int>::iterator it = _encodeLoadMap.find(ip);
//   if (it == _encodeLoadMap.end()) _encodeLoadMap.insert(make_pair(ip, load));
//   else _encodeLoadMap[ip] += load;
//   _lockELMap.unlock();
// }
// 
// int StripeStore::getDataLoad(unsigned int ip) {
//   int toret;
//   _lockDLMap.lock();
//   unordered_map<unsigned int, int>::iterator it = _dataLoadMap.find(ip);
//   if (it == _dataLoadMap.end()) toret = 0;
//   else toret = _dataLoadMap[ip];
//   _lockDLMap.unlock();
//   return toret;
// }
// 
// int StripeStore::getControlLoad(unsigned int ip) {
//   int toret;
//   _lockCLMap.lock();
//   unordered_map<unsigned int, int>::iterator it = _controlLoadMap.find(ip);
//   if (it == _controlLoadMap.end()) toret = 0;
//   else toret = _controlLoadMap[ip];
//   _lockCLMap.unlock();
//   return toret;
// }
// 
// int StripeStore::getRepairLoad(unsigned int ip) {
//   int toret;
//   _lockRLMap.lock();
//   unordered_map<unsigned int, int>::iterator it = _repairLoadMap.find(ip);
//   if (it == _repairLoadMap.end()) toret = 0;
//   else toret = _repairLoadMap[ip];
//   _lockRLMap.unlock();
//   return toret;
// }
// 
// int StripeStore::getEncodeLoad(unsigned int ip) {
//   int toret;
//   _lockELMap.lock();
//   unordered_map<unsigned int, int>::iterator it = _encodeLoadMap.find(ip);
//   if (it == _encodeLoadMap.end()) toret = 0;
//   else toret = _encodeLoadMap[ip];
//   _lockELMap.unlock();
//   return toret;
// }
// 
// bool StripeStore::poolExists(string poolname) {
//   bool toret;
//   _lockECPoolMap.lock(); 
//   unordered_map<string, OfflineECPool*>::iterator it = _offlineECPoolMap.find(poolname);
//   if (it != _offlineECPoolMap.end()) toret = true;
//   else toret = false;
//   _lockECPoolMap.unlock();
//   return toret;
// }
// 
// OfflineECPool* StripeStore::getECPool(string poolname, ECPolicy* ecpolicy) {
//   OfflineECPool* toret;
//   _lockECPoolMap.lock();
//   unordered_map<string, OfflineECPool*>::iterator it = _offlineECPoolMap.find(poolname);
//   if (it != _offlineECPoolMap.end()) toret = _offlineECPoolMap[poolname];
//   else {
//     toret = new OfflineECPool(poolname, ecpolicy);
//     _offlineECPoolMap.insert(make_pair(poolname, toret));
//   }
//   _lockECPoolMap.unlock();
//   return toret;
// }
// 
// OfflineECPool* StripeStore::getECPool(string poolname) {
//   // TODO: the poolname must exist!
//   OfflineECPool* toret;
//   _lockECPoolMap.lock();
//   unordered_map<string, OfflineECPool*>::iterator it = _offlineECPoolMap.find(poolname);
//   assert (it != _offlineECPoolMap.end());
//   _lockECPoolMap.unlock();
//   return _offlineECPoolMap[poolname];
// }
// 
// void StripeStore::addECPool(OfflineECPool* ecpool) {
//   _offlineECPoolMap.insert(make_pair(ecpool->_poolName, ecpool));
// }
// 
// void StripeStore::addToECQueue(string poolname, string stripename) {
//   _lockPECQueue.lock();
//   _pendingECQueue.push(make_pair(poolname, stripename));
//   _lockPECQueue.unlock();
// }
// 
// void StripeStore::addToLostMap(string objname) {
//   // check whether objname is in _RPInProgress
//   bool inrepair = false;
//   _lockRPInProgress.lock();
//   if (find(_RPInProgress.begin(), _RPInProgress.end(), objname) != _RPInProgress.end()) {
//     inrepair = true;
//   }
//   _lockRPInProgress.unlock();
// 
//   if (inrepair) return;
// 
//   _lockLostMap.lock();
//   if (_lostMap.find(objname) == _lostMap.end()) {
//     _lostMap.insert(make_pair(objname, 1));
//   } else {
//     _lostMap[objname]++;
//   }
//   _lockLostMap.unlock();
// }
// 
// int StripeStore::getRPInProgressNum() {
//   _lockRPInProgress.lock();
//   int toret = _RPInProgress.size();
//   _lockRPInProgress.unlock();
//   return toret;
// }
// 
void StripeStore::scanRepair() {
//   int concurrentNum = _conf->_ec_concurrent;
  while (true) {
//     std::this_thread::sleep_for(std::chrono::seconds(1));
//     if (!_enableRepair) continue;
//     cout << "StripeStore::scanning repair queue" << endl;
//     int rpInProgressNum = getRPInProgressNum();
//     cout << "StripeStore::scanRepair.rpInProgressNum = " << rpInProgressNum << endl;
//     while (_lostMap.size() && (rpInProgressNum < concurrentNum)) {
//       string objname;
// 
//       // search the lost map and find the one with the most request num
//       int maxreq=0;
//       _lockLostMap.lock();
//       for (auto item: _lostMap) {
//         if (item.second > maxreq) {
//           maxreq = item.second;
//           objname = item.first;
//         }
//       }
//       _lockLostMap.unlock();
// 
//       // now we have the obj with the most request num
//       if (_conf->_repair_scheduling == "threshold") {
//         if (!_enableRepair) continue;
//         if (maxreq < _conf->_repair_threshold) continue;
//       }
// 
//       if ((_conf->_repair_scheduling == "delay" ) && 
//           !_enableRepair) continue;
// 
// //      if (_conf->_repair_scheduling == "normal") continue;
//     
//       // send repair request to coordinator  
//       CoorCommand* coorCmd = new CoorCommand();
//       coorCmd->buildType8(8, _conf->_localIp, objname);
//       coorCmd->sendTo(_conf->_coorIp);
// 
//       delete coorCmd;
// 
//       // now we move the obj to RPInProgress
//       startRepair(objname);
//       _lockLostMap.lock();
//       _lostMap.erase(_lostMap.find(objname));
//       _lockLostMap.unlock();
// 
//       rpInProgressNum = getRPInProgressNum();
//     }
  }
}

// int StripeStore::getECInProgressNum() {
//   _lockECInProgress.lock();
//   int toret = _ECInProgress.size();
//   _lockECInProgress.unlock();
//   return toret;
// }
// 
void StripeStore::scanning() {
  // TODO: we may add some variable to enable scanning
//   int concurrentNum = _conf->_ec_concurrent;
  while (true) {
//     std::this_thread::sleep_for (std::chrono::seconds(1));
//     if (!_enableScan) continue;
//     cout << "StripeStore::scanning the pendingECQueue" << endl;
//     _lockPECQueue.lock();
//     int ecInProgressNum = getECInProgressNum();
//     cout << "StripeStore::scanning.ecInProgressNum = " << ecInProgressNum << endl;
//     while (_pendingECQueue.getSize() && ecInProgressNum < concurrentNum) {
//     
//       pair<string, string> curpair = _pendingECQueue.pop();
//     
//       string poolname = curpair.first;
//       string stripename = curpair.second;
//       cout << "StripeStore::scanning.poolname: " << poolname << ", stripename: " << stripename << endl;
//       startECStripe(stripename);
// 
//       // send offline enc request to coordinator
//       CoorCommand* coorCmd = new CoorCommand();
//       coorCmd->buildType4(4, _conf->_localIp, poolname, stripename);
//       coorCmd->sendTo(_conf->_coorIp);
// 
//       delete coorCmd;
// 
//       ecInProgressNum = getECInProgressNum();
// 
//     }
//     _lockPECQueue.unlock();
  }
}

// void StripeStore::startECStripe(string stripename) {
//   _lockECInProgress.lock();
//   if (_ECInProgress.size() == 0) gettimeofday(&_startEnc, NULL);
//   _ECInProgress.push_back(stripename);
//   _lockECInProgress.unlock();
// }
// 
// void StripeStore::finishECStripe(string stripename) {
//   _lockECInProgress.lock();
//   vector<string>::iterator pos = find(_ECInProgress.begin(), _ECInProgress.end(), stripename);
//   if (pos != _ECInProgress.end()) _ECInProgress.erase(pos);
//   if (_ECInProgress.size() == 0) {
//     gettimeofday(&_endEnc, NULL);
//     cout << "StripeStore::finishECStripe.encodeTime = " << RedisUtil::duration(_startEnc, _endEnc) << endl;
//   }
//   _lockECInProgress.unlock();
// }
// 
// int StripeStore::getRandomInt(int size) {
//   _lockRandom.lock();
//   int toret = rand() % size;
//   _lockRandom.unlock();
//   return toret;
// }
// 
// void StripeStore::setScan(bool status) {
//   _enableScan = status;
// }
// 
// void StripeStore::setRepair(bool status) {
//   _enableRepair = status;
// }
// 
// void StripeStore::startRepair(string objname) {
//   _lockRPInProgress.lock();
//   _RPInProgress.push_back(objname);
//   _lockRPInProgress.unlock();
// }
// 
// void StripeStore::finishRepair(string objname) {
//   _lockRPInProgress.lock();
//   vector<string>::iterator pos = find(_RPInProgress.begin(), _RPInProgress.end(), objname);
//   if (pos != _RPInProgress.end()) _RPInProgress.erase(pos);
//   _lockRPInProgress.unlock();
// }
