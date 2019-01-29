#!/bin/bash

QFS_SRC_DIR=/home/openec/qfs

# 1. copy openec integrations
cp src/Reader.cc $QFS_SRC_DIR/src/cc/libclient/Reader.cc
cp src/LayoutManager.cc $QFS_SRC_DIR/src/cc/meta/LayoutManager.cc
cp src/LayoutManager.h $QFS_SRC_DIR/src/cc/meta/LayoutManager.h
cp src/MetaRequest.cc $QFS_SRC_DIR/src/cc/meta/MetaRequest.cc
cp src/cpfromqfs_main.cc $QFS_SRC_DIR/src/cc/tools/cpfromqfs_main.cc
cp src/CoorCommand.cc $QFS_SRC_DIR/src/cc/common/CoorCommand.cc
cp src/CoorCommand.h $QFS_SRC_DIR/src/cc/common/CoorCommand.h
cp src/cptoqfs_main.cc $QFS_SRC_DIR/src/cc/tools/cptoqfs_main.cc
cp src/CMakeLists.txt $QFS_SRC_DIR/src/cc/common/CMakeLists.txt

# 2. compile
cd $QFS_SRC_DIR
make
