#include "SSEntry.hh"

SSEntry::SSEntry(string filename, int type, int filesizeMB, string ecidpool, vector<string> objname, vector<unsigned int> loc) {
  _filename = filename;
  _type = type;
  _filesizeMB = filesizeMB;
  _ecidpool = ecidpool;
  _objList = objname;
  _objLoc = loc;
}

SSEntry::SSEntry(string line) {
//  int start = 0;
//  int pos = line.find_first_of(";");
//  vector<string> entryitems;
//  while (pos != string::npos) {
//    string item = line.substr(start, pos - start);
//    start = pos + 1;
//    pos = line.find_first_of(";", start);
//    entryitems.push_back(item);
//  }
//  string item = line.substr(start, pos - start);
//  entryitems.push_back(item);
//  for (int i=0; i<entryitems.size(); i++) cout << entryitems[i] << endl;
  vector<string> entryitems = RedisUtil::str2container(line);
  
  // entryitems[0]:filename
  _filename = entryitems[0];
  // entryitems[1]:type
  _type = stoi(entryitems[1]);
  // entryitems[2]:filesizeMB
  _filesizeMB = stoi(entryitems[2]);
  // entryitems[3]:ecidpool
  _ecidpool = entryitems[3];
  // remain: objs
  int objnum = (entryitems.size() - 4) / 2;
  for (int i=0; i<objnum; i++) {
    int idx = 4 + i * 2;
    string objname = entryitems[idx];
    string objlocstr = entryitems[idx+1];
    _objList.push_back(objname);
    unsigned long loc = stoul(objlocstr, nullptr, 0);
    unsigned int objloc = (unsigned int)loc;
    _objLoc.push_back(objloc);
  }
}

string SSEntry::getFilename() {
  return _filename;
}

int SSEntry::getType() {
  return _type;
}

int SSEntry::getFilesizeMB() {
  return _filesizeMB;
}

string SSEntry::getEcidpool() {
  return _ecidpool;
}

vector<string> SSEntry::getObjlist() {
  return _objList;
}

vector<unsigned int> SSEntry::getObjloc() {
  return _objLoc;
}

int SSEntry::getIdxOfObj(string objname) {
  int toret=-1;
  for (int i=0; i<_objList.size(); i++) {
    if (objname == _objList[i]) {
      toret = i;
      break;
    }
  }
  assert (toret != -1);
  return toret;
}

unsigned int SSEntry::getLocOfObj(string objname) {
  unsigned int toret;
  bool find=false;
  for (int i=0; i<_objList.size(); i++) {
    if (objname == _objList[i]) {
      toret = _objLoc[i];
      find = true;
      break;
    }
  }
  assert (find);
  return toret;
}

void SSEntry::updateObjLoc(string objname, unsigned int loc) {
  _updateLock.lock();
  int idx=-1;
  for (int i=0; i<_objList.size(); i++) {
    if (_objList[i] == objname) {
      idx = i;
      break;
    }
  }
  if (idx != -1) _objLoc[idx] = loc;
  _updateLock.unlock();
  assert(idx != -1);
}

string SSEntry::toString() {
  string toret = "";
  toret += _filename+";";
  toret += to_string(_type)+";";
  toret += to_string(_filesizeMB)+";";
  toret += _ecidpool+";";
  int num = _objList.size();
  for (int i=0; i<num; i++) {
    string obj = _objList[i];
    toret += obj+";";
    unsigned int loc = _objLoc[i];
    toret += to_string(loc)+";";
  }
  toret += "\n";
  return toret;
}

void SSEntry::dump() {
  cout << "SSEntry:: filename: "<< _filename << ", type: " << _type << ", filesizeMB: " << _filesizeMB 
       << ", ecidpool: " << _ecidpool << ", objname: ";
  for (int i=0; i<_objList.size(); i++) cout << _objList[i] << " ";
  cout << ", objloc: ";
  for (int i=0; i<_objLoc.size(); i++) cout << RedisUtil::ip2Str(_objLoc[i]) << " ";
  cout << endl;
}
