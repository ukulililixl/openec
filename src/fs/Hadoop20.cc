#include "Hadoop20.hh"

Hadoop20::Hadoop20(vector<string> params, Config* conf) {
  cout << "Hadoop20::Hadoop20" << endl;
  _ip = params[0];
  _port = atoi(params[1].c_str());
  _fs = hdfsConnect(_ip.c_str(), _port);

  if (!_fs) {
    cerr << "Failed to connect to hadoop20!\n" << endl;
    exit(-1);
  }

  _conf = conf;
}

Hadoop20::~Hadoop20() {
  cout << "Hadoop20::~Hadoop20" << endl;
  hdfsDisconnect(_fs);
}

Hadoop20File* Hadoop20::openFile(string filename, string mode) {
  Hadoop20File* toret;
  hdfsFile underfile;
  if (mode == "read") {
    cout << "Hadoop20::openFile " << filename << " for read" << endl;
    underfile = hdfsOpenFile(_fs, filename.c_str(), O_RDONLY, _conf->_pktSize, 0, 0);
    if (underfile) {
      // try to read 1 byte
      int tmpres;
      int res = hdfsRead(_fs, underfile, (void*)&tmpres, 1);
      cout << "Hadoop20::openFile.try to read 1 byte, res: " << res << endl;
      if (res < 1) underfile = NULL;
      else hdfsSeek(_fs, underfile, 0);
    }
  } else {
    cout << "Hadoop20::openFile "  << filename << " for write" << endl;
    underfile = hdfsOpenFile(_fs, filename.c_str(), O_WRONLY |O_CREAT|O_WRONLY, 0, 0, 0);
  }
  if (!underfile) {
    cerr << "Failed to open " << filename << " in Hadoop20" << endl;
    toret = NULL;
  } else {
    toret = new Hadoop20File(filename, underfile);
  }
  return toret;
}

void Hadoop20::writeFile(UnderFile* file, char* buffer, int len) {
  hdfsWrite(_fs, ((Hadoop20File*)file)->_objfile, (void*)buffer, len);
}

void Hadoop20::flushFile(UnderFile* file) {
  hdfsFlush(_fs, ((Hadoop20File*)file)->_objfile);
}

void Hadoop20::closeFile(UnderFile* file) {
  cout << "Hadoop20::closeFile"  << endl;
  hdfsFile objfile = ((Hadoop20File*)file)->_objfile;
  if (objfile) {
    hdfsCloseFile(_fs, objfile);
    ((Hadoop20File*)file)->_objfile = NULL;
  }
  delete file;
}

int Hadoop20::readFile(UnderFile* file, char* buffer, int len) {
  return hdfsRead(_fs, ((Hadoop20File*)file)->_objfile, (void*)buffer, len);
}

int Hadoop20::pReadFile(UnderFile* file, int offset, char* buffer, int len) {
  return hdfsPread(_fs, ((Hadoop20File*)file)->_objfile, offset, (void*)buffer, len);
}

int Hadoop20::getFileSize(UnderFile* file) {
  hdfsFileInfo* fileinfo = hdfsGetPathInfo(_fs, ((Hadoop20File*)file)->_objname.c_str());
  return fileinfo->mSize;
}

