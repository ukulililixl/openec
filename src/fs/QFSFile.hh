#ifndef _QFSFILE_HH_
#define _QFSFILE_HH_

#include "UnderFile.hh"
#include "../inc/include.hh"

class QFSFile : public UnderFile {
  public:
    string _objname;
    int _fd;

    QFSFile(string objname, int fd);
    ~QFSFile();
    int getFileSize();
};

#endif
