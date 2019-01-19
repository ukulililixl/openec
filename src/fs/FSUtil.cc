#include "FSUtil.hh"

UnderFS* FSUtil::createFS(string type, vector<string> param, Config* conf) { 
  UnderFS* toret;
  if (type == "hdfs3") {
    #ifdef HDFS3
    cout << "FSUtil::hdfs3" << endl;
    toret = new Hadoop3(param, conf);
    #endif
  } else if (type == "hdfsraid") {
    #ifdef HDFSRAID
    cout << "FSUtil::hdfsraid" << endl;
    toret = new Hadoop20(param, conf);
    #endif
  } else if (type == "qfs") {
    #ifdef QFS
    cout << "FSUtil::qfs" << endl;
    toret = new QFS(param, conf);
    #endif
  } else {
    cout << "unrecognized FS type!" << endl;
    toret = NULL;
  }

  return toret;
}

void FSUtil::deleteFS(string type, UnderFS* fshandler) {
  if (type == "hdfs3") {
    #ifdef HDFS3
    cout << "FSUtil::hdfs3" << endl;
    delete (Hadoop3*)fshandler;
    #endif
  } else if (type == "hdfsraid") {
    #ifdef HDFSRAID
    cout << "FSUtil::hdfsraid" << endl;
    delete (Hadoop20*)fshandler;
    #endif
  } else if (type == "qfs") {
    #ifdef QFS
    cout << "FSUtil::qfs" << endl;
    delete (QFS*)fshandler;
    #endif
  }
}
