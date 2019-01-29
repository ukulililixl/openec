#include "FSUtil.hh"

UnderFS* FSUtil::createFS(string type, vector<string> param, Config* conf) { 
  UnderFS* toret;
  if (type == "HDFS3") {
    #ifdef HDFS3
    cout << "FSUtil::HDFS3" << endl;
    toret = new Hadoop3(param, conf);
    #endif
  } else if (type == "HDFSRAID") {
    #ifdef HDFSRAID
    cout << "FSUtil::HDFSRAID" << endl;
    toret = new Hadoop20(param, conf);
    #endif
  } else if (type == "QFS") {
    #ifdef QFS
    cout << "FSUtil::QuantcastFS" << endl;
    toret = new QuantcastFS(param, conf);
    #endif
  } else {
    cout << "unrecognized FS type!" << endl;
    toret = NULL;
  }

  return toret;
}

void FSUtil::deleteFS(string type, UnderFS* fshandler) {
  if (type == "HDFS3") {
    #ifdef HDFS3
    cout << "FSUtil::HDFS3" << endl;
    delete (Hadoop3*)fshandler;
    #endif
  } else if (type == "HDFSRAID") {
    #ifdef HDFSRAID
    cout << "FSUtil::HDFSRAID" << endl;
    delete (Hadoop20*)fshandler;
    #endif
  } else if (type == "QFS") {
    #ifdef QFS
    cout << "FSUtil::QuantcastFS" << endl;
    delete (QuantcastFS*)fshandler;
    #endif
  }
}
