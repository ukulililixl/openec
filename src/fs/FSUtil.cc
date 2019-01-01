#include "FSUtil.hh"

UnderFS* FSUtil::createFS(string type, vector<string> param, Config* conf) { 
  UnderFS* toret;
  if (type == "hadoop3") {
    cout << "hadoop3" << endl;
    toret = new Hadoop3(param, conf);
//  } else if (type == "hadoop20") {
//    cout << "hadoop20" << endl;
//    toret = new Hadoop20(param, conf);
  } else if (type == "qfs") {
    cout << "qfs" << endl;
    toret = new QFS(param, conf);
  } else {
    cout << "unrecognized FS type!" << endl;
//    toret = new Hadoop3(param, conf);
    toret = NULL;
  }

  return toret;
}

void FSUtil::deleteFS(string type, UnderFS* fshandler) {
  if (type == "hadoop3") {
    cout << "hadoop3" << endl;
    delete (Hadoop3*)fshandler;
//  } else if (type == "hadoop20") {
//    cout << "hadoop20" << endl;
//    delete (Hadoop20*)fshandler;
  }
}
