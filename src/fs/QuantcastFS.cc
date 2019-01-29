#include "QuantcastFS.hh"

QuantcastFS::QuantcastFS(vector<string> params, Config* conf) {
  _ip = params[0];
  _port = atoi(params[1].c_str());
  _fs = KFS::Connect(_ip, _port);
  if (!_fs) {
    cout << "fail to connect qfs" << endl;
  }
}

QuantcastFS::~QuantcastFS() {
  delete _fs;
}

QFSFile* QuantcastFS::openFile(string filename, string mode) {
  QFSFile* toret = NULL;
  int fd;
  if (mode == "read") {
    cout << "QuantcastFS::openFile " << filename << " for read" << endl;
    if ((fd = _fs->Open(filename.c_str(), O_RDONLY)) < 0) {
      cout << "QuantcastFS::openFile error!" << endl;
    } else {
      cout << "QuantcastFS::openFile.fd: " << fd << endl;
      // try to read 1 byte?
      int tmpres;
      int res = _fs->Read(fd, (char*)&tmpres, 4);
      if (res != 4) {
        cout << "QuantcastFS::openFile error!" << endl; 
        _fs->Close(fd);
      } else {
        // reset offset
        _fs->Seek(fd, 0);
        toret = new QFSFile(filename, fd);
      }
    }
  } else {
    cout << "QuantcastFS::openFile " << filename << " for write" << endl;
    if ((fd = _fs->Create(filename.c_str(), 1)) < 0) {
      cout << "QuantcastFS::openFile error!" << endl;
    } else {
      cout << "QuantcastFS::openFile.fd: " << fd << endl;
      toret = new QFSFile(filename, fd);
    }
  }
  return toret;
}

void QuantcastFS::writeFile(UnderFile* file, char* buffer, int len) {
  int fd = ((QFSFile*)file)->_fd;
  _fs->Write(fd, buffer, len);
}

void QuantcastFS::flushFile(UnderFile* file) {
  int fd = ((QFSFile*)file)->_fd;
  _fs->Sync(fd);
}

void QuantcastFS::closeFile(UnderFile* file) {
  int fd = ((QFSFile*)file)->_fd;
  _fs->Close(fd);
}

int QuantcastFS::readFile(UnderFile* file, char* buffer, int len) {
  int fd = ((QFSFile*)file)->_fd;
  return _fs->Read(fd, buffer, len);
}

int QuantcastFS::pReadFile(UnderFile* file, int offset, char* buffer, int len) {
  int fd = ((QFSFile*)file)->_fd;
  _fs->Seek(fd, offset);
  return _fs->Read(fd, buffer, len);
}

int QuantcastFS::getFileSize(UnderFile* file) {
  KFS::KfsFileAttr fileAttr;
  string filename = ((QFSFile*)file)->_objname;
  _fs->Stat(filename.c_str(), fileAttr);
  long size = fileAttr.fileSize;
  return (int)size;
}
