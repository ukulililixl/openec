#include "QFS.hh"

QFS::QFS(vector<string> params, Config* conf) {
  _ip = params[0];
  _port = atoi(params[1].c_str());
  _fs = KFS::Connect(_ip, _port);
  if (!_fs) {
    cout << "fail to connect qfs" << endl;
  }
}

QFS::~QFS() {
}

QFSFile* QFS::openFile(string filename, string mode) {
  QFSFile* toret = NULL;
  int fd;
  if (mode == "read") {
    cout << "QFS::openFile " << filename << " for read" << endl;
    if ((fd = _fs->Open(filename.c_str(), O_RDONLY)) < 0) {
      cout << "QFS::openFile error!" << endl;
    } else {
      cout << "QFS::openFile.fd: " << fd << endl;
      // try to read 1 byte?
      int tmpres;
      int res = _fs->Read(fd, (char*)&tmpres, 4);
      if (res != 4) {
        cout << "QFS::openFile error!" << endl; 
        _fs->Close(fd);
      } else {
        // reset offset
        _fs->Seek(fd, 0);
        toret = new QFSFile(filename, fd);
      }
    }
  } else {
    cout << "QFS::openFile " << filename << " for write" << endl;
    if ((fd = _fs->Create(filename.c_str(), 1)) < 0) {
      cout << "QFS::openFile error!" << endl;
    } else {
      cout << "QFS::openFile.fd: " << fd << endl;
      toret = new QFSFile(filename, fd);
    }
  }
  return toret;
}

void QFS::writeFile(UnderFile* file, char* buffer, int len) {
  int fd = ((QFSFile*)file)->_fd;
  _fs->Write(fd, buffer, len);
}

void QFS::flushFile(UnderFile* file) {
  int fd = ((QFSFile*)file)->_fd;
  _fs->Sync(fd);
}

void QFS::closeFile(UnderFile* file) {
  int fd = ((QFSFile*)file)->_fd;
  _fs->Close(fd);
}

int QFS::readFile(UnderFile* file, char* buffer, int len) {
  int fd = ((QFSFile*)file)->_fd;
  return _fs->Read(fd, buffer, len);
}

int QFS::pReadFile(UnderFile* file, int offset, char* buffer, int len) {
  int fd = ((QFSFile*)file)->_fd;
  _fs->Seek(fd, offset);
  return _fs->Read(fd, buffer, len);
}

int QFS::getFileSize(UnderFile* file) {
  KFS::KfsFileAttr fileAttr;
  string filename = ((QFSFile*)file)->_objname;
  _fs->Stat(filename.c_str(), fileAttr);
  long size = fileAttr.fileSize;
  return (int)size;
}
