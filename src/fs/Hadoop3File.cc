#include "Hadoop3File.hh"

Hadoop3File::Hadoop3File(string filename, hdfsFile file) {
  _objname = filename;
  _objfile = file;
}
