#!/bin/bash

HADOOP_SRC_DIR=/home/openec/hadoop-20
OECINTEG=`pwd`
OECLIB=$OECINTEG/../lib

# 0. prepare lib
cp $OECLIB/* ./

# 1. compile java version of openec command
javac -cp .:./commons-pool2-2.4.2.jar:./commons-pool2-2.4.2-javadoc.jar:./jedis-3.0.0-SNAPSHOT.jar oec/protocol/AGCommand.java oec/protocol/CoorCommand.java
jar cf oec-fs-protocol.jar oec/protocol/AGCommand.class oec/protocol/CoorCommand.class

# 2. copy libraries to hadoop src 
cp commons-pool2-2.4.2.jar $HADOOP_SRC_DIR/lib
cp commons-pool2-2.4.2-javadoc.jar $HADOOP_SRC_DIR/lib
cp jedis-3.0.0-SNAPSHOT.jar $HADOOP_SRC_DIR/lib
cp oec-fs-protocol.jar $HADOOP_SRC_DIR/lib
cp hadoop-0.20-raid.jar $HADOOP_SRC_DIR/lib

# 3. copy openec modifications to hadoop src
cp src/HadoopPipes.cc $HADOOP_SRC_DIR/src/c++/pipes/impl/HadoopPipes.cc
cp src/DistBlockIntegrityMonitor.java $HADOOP_SRC_DIR/src/contrib/raid/src/java/org/apache/hadoop/raid/DistBlockIntegrityMonitor.java
cp src/DistRaidNode.java $HADOOP_SRC_DIR/src/contrib/raid/src/java/org/apache/hadoop/raid/DistRaidNode.java
cp src/PlacementMonitor.java $HADOOP_SRC_DIR/src/contrib/raid/src/java/org/apache/hadoop/raid/PlacementMonitor.java
cp src/PurgeMonitor.java $HADOOP_SRC_DIR/src/contrib/raid/src/java/org/apache/hadoop/raid/PurgeMonitor.java
cp src/RaidNode.java $HADOOP_SRC_DIR/src/contrib/raid/src/java/org/apache/hadoop/raid/RaidNode.java
cp src/NetworkTopology.java $HADOOP_SRC_DIR/src/core/org/apache/hadoop/net/NetworkTopology.java
cp src/BlockPlacementPolicyOEC.java $HADOOP_SRC_DIR/src/hdfs/org/apache/hadoop/hdfs/server/namenode/BlockPlacementPolicyOEC.java
cp src/FSNamesystem.java $HADOOP_SRC_DIR/src/hdfs/org/apache/hadoop/hdfs/server/namenode/FSNamesystem.java
cp src/NameNode.java $HADOOP_SRC_DIR/src/hdfs/org/apache/hadoop/hdfs/server/namenode/NameNode.java
cp src/DFSInputStream.java $HADOOP_SRC_DIR/src/hdfs/org/apache/hadoop/hdfs/DFSInputStream.java
cp src/Makefile.am $HADOOP_SRC_DIR/src/native/lib/Makefile.am
cp src/Makefile.in $HADOOP_SRC_DIR/src/native/lib/Makefile.in
cp src/NativeJerasure.c $HADOOP_SRC_DIR/src/native/src/org/apache/hadoop/util/NativeJerasure.c
cp src/NativeJerasure.h $HADOOP_SRC_DIR/src/native/src/org/apache/hadoop/util/NativeJerasure.h
cp src/build.xml $HADOOP_SRC_DIR/

# 4. compile
cd $HADOOP_SRC_DIR
ant -Dversion=0.20 -Dcompile.native=true -Dcompile.c++=true -Dlibhdfs=true compile-c++-libhdfs clean jar bin-package

mkdir -p ${HADOOP_SRC_DIR}/lib/native
cp ${HADOOP_SRC_DIR}/build/hadoop-0.20/lib/hadoop-0.20-raid.jar ${HADOOP_SRC_DIR}/lib/native/
cp ${HADOOP_SRC_DIR}/build/native/Linux-amd64-64/lib/libhadoop.* ${HADOOP_SRC_DIR}/lib/native/
cp ${HADOOP_SRC_DIR}/build/c++/Linux-amd64-64/lib/* ${HADOOP_SRC_DIR}/lib/native/
cp ${HADOOP_SRC_DIR}/build/native/Linux-amd64-64/lib/libhadoopsnappy.* ${HADOOP_SRC_DIR}/lib/native
cp ${HADOOP_SRC_DIR}/build/native/Linux-amd64-64/lib/libhadoop.* ${HADOOP_SRC_DIR}/lib/native
