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

void SSEntry::dump() {
  cout << "SSEntry:: filename: "<< _filename << ", type: " << _type << ", filesizeMB: " << _filesizeMB 
       << ", ecidpool: " << _ecidpool << ", objname: ";
  for (int i=0; i<_objList.size(); i++) cout << _objList[i] << " ";
  cout << ", objloc: ";
  for (int i=0; i<_objLoc.size(); i++) cout << RedisUtil::ip2Str(_objLoc[i]) << " ";
  cout << endl;
}
