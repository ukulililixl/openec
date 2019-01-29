#include "QFSFile.hh"

QFSFile::QFSFile(string objname, int fd) {
  _objname = objname;
  _fd = fd;  
}

QFSFile::~QFSFile(){}

int QFSFile::getFileSize() {
  return 0;
}
