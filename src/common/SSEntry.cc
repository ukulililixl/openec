#include "SSEntry.hh"

SSEntry::SSEntry(string filename, int type, int filesizeMB, string ecidpool, vector<string> objname, vector<unsigned int> loc) {
  _filename = filename;
  _type = type;
  _filesizeMB = filesizeMB;
  _ecidpool = ecidpool;
  _objList = objname;
  _objLoc = loc;
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

void SSEntry::dump() {
  cout << "SSEntry:: filename: "<< _filename << ", type: " << _type << ", filesizeMB: " << _filesizeMB 
       << ", ecidpool: " << _ecidpool << ", objname: ";
  for (int i=0; i<_objList.size(); i++) cout << _objList[i] << " ";
  cout << ", objloc: ";
  for (int i=0; i<_objLoc.size(); i++) cout << RedisUtil::ip2Str(_objLoc[i]) << " ";
  cout << endl;
}
