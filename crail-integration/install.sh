#!/bin/bash

CRAIL_SRC_DIR=/home/openec/crail

# 0. cp third party libraries
cp ../lib/* ./

# 1. compile java version of openec command
javac -cp .:./commons-pool2-2.4.2.jar:./commons-pool2-2.4.2-javadoc.jar:./jedis-3.0.0-SNAPSHOT.jar oec/protocol/AGCommand.java oec/protocol/CoorCommand.java
jar cf oec-fs-protocol.jar oec/protocol/AGCommand.class oec/protocol/CoorCommand.class

# 2. copy libraries to crail src
mkdir -p $CRAIL_SRC_DIR/oeclib
cp commons-pool2-2.4.2.jar $CRAIL_SRC_DIR/oeclib
cp commons-pool2-2.4.2-javadoc.jar $CRAIL_SRC_DIR/oeclib
cp jedis-3.0.0-SNAPSHOT.jar $CRAIL_SRC_DIR/oeclib
cp oec-fs-protocol.jar $CRAIL_SRC_DIR/oeclib

# 3. copy openec integrations
cp CrailConstants.java ${CRAIL_SRC_DIR}/client/src/main/java/org/apache/crail/conf/CrailConstants.java
cp FileName.java ${CRAIL_SRC_DIR}/client/src/main/java/org/apache/crail/metadata/FileName.java
cp CrailUtils.java ${CRAIL_SRC_DIR}/client/src/main/java/org/apache/crail/utils/CrailUtils.java
cp pom.xml ${CRAIL_SRC_DIR}/namenode/pom.xml
cp BlockStore.java ${CRAIL_SRC_DIR}/namenode/src/main/java/org/apache/crail/namenode/BlockStore.java
cp NameNodeService.java ${CRAIL_SRC_DIR}/namenode/src/main/java/org/apache/crail/namenode/NameNodeService.java
cp StorageRpcClient.java ${CRAIL_SRC_DIR}/storage/src/main/java/org/apache/crail/storage/StorageRpcClient.java
cp StorageServer.java ${CRAIL_SRC_DIR}/storage/src/main/java/org/apache/crail/storage/StorageServer.java
cp HddStorageServer.java ${CRAIL_SRC_DIR}/storage-narpc/src/main/java/org/apache/crail/storage/tcp/HddStorageServer.java
cp HddStorageTier.java ${CRAIL_SRC_DIR}/storage-narpc/src/main/java/org/apache/crail/storage/tcp/HddStorageTier.java
cp TcpStorageConstants.java ${CRAIL_SRC_DIR}/storage-narpc/src/main/java/org/apache/crail/storage/tcp/TcpStorageConstants.java
cp TcpStorageServer.java ${CRAIL_SRC_DIR}/storage-narpc/src/main/java/org/apache/crail/storage/tcp/TcpStorageServer.java

# 4. compile source code
cd $CRAIL_SRC_DIR
mvn -DskipTests install
