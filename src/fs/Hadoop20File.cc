#include "Hadoop20File.hh"

Hadoop20File::Hadoop20File(string filename, hdfsFile file) {
  _objname = filename;
  _objfile = file;
}

