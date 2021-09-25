#!/bin/bash

HADOOP_SRC_DIR=/home/openec/hadoop-3.0.0-src

# 0. cp third party libraries
cp ../lib/* ./

# 1. compile java version of openec command
javac -cp .:./commons-pool2-2.4.2.jar:./commons-pool2-2.4.2-javadoc.jar:./jedis-3.0.0-SNAPSHOT.jar oec/protocol/AGCommand.java oec/protocol/CoorCommand.java
jar cf oec-fs-protocol.jar oec/protocol/AGCommand.class oec/protocol/CoorCommand.class

# 2. copy libraries to hadoop src
mkdir -p $HADOOP_SRC_DIR/oeclib
cp commons-pool2-2.4.2.jar $HADOOP_SRC_DIR/oeclib
cp commons-pool2-2.4.2-javadoc.jar $HADOOP_SRC_DIR/oeclib
cp jedis-3.0.0-SNAPSHOT.jar $HADOOP_SRC_DIR/oeclib
cp oec-fs-protocol.jar $HADOOP_SRC_DIR/oeclib

# 3. copy openec modifications to hadoop src
cp src/BlockManager.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/
cp src/BlockPlacementPolicyDefault.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/
cp src/BlockPlacementPolicy.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/
cp src/BlockPlacementPolicyOEC.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/
cp src/CommandWithDestination.java $HADOOP_SRC_DIR/hadoop-common-project/hadoop-common/src/main/java/org/apache/hadoop/fs/shell/
cp src/FSDirWriteFileOp.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/namenode/
cp src/FSNamesystem.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/namenode/
cp src/LowRedundancyBlocks.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/
cp src/NameNodeRpcServer.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/namenode/
cp src/NetworkTopology.java $HADOOP_SRC_DIR/hadoop-common-project/hadoop-common/src/main/java/org/apache/hadoop/net/
cp src/hdfs.c  $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs-native-client/src/main/native/libhdfs/hdfs.c
cp src/DFSInputStream.java $HADOOP_SRC/DIR/hadoop-hdfs-project/hadoop-hdfs-client/src/main/java/org/apache/hadoop/hdfs/
cp src/pom.xml $HADOOP_SRC_DIR/

# 4. prepare pom for hadoop-hdfs-client package
oecprotopath=${HADOOP_SRC_DIR}/oeclib/oec-fs-protocol.jar
dependencyoec="<dependency>\n<groupId>oec.protocol</groupId>\n<artifactId>oec</artifactId>\n<version>1.0.0-SNAPSHOT</version>\n<scope>system</scope>\n<systemPath>${oecprotopath}</systemPath>\n</dependency>\n"
pomfile=${HADOOP_SRC_DIR}/hadoop-hdfs-project/hadoop-hdfs-client/pom.xml
sed -i "/<dependencies>/a ${dependencyoec}" $pomfile

redisclipath=${HADOOP_SRC_DIR}/oeclib/jedis-3.0.0-SNAPSHOT.jar
dependencyredis="<dependency>\n<groupId>redis.clients</groupId>\n<artifactId>jedis</artifactId>\n<version>3.0.0-SNAPSHOT</version>\n<scope>system</scope>\n<systemPath>${redisclipath}</systemPath>\n</dependency>\n"
sed -i "/<dependencies>/a ${dependencyredis}" $pomfile

commonpath=${HADOOP_SRC_DIR}/oeclib/commons-pool2-2.4.2.jar
dependencycommon="<dependency>\n<groupId>org.apache.commons</groupId>\n<artifactId>commons-pool2</artifactId>\n<version>2.4.2</version>\n<scope>system</scope>\n<systemPath>${commonpath}</systemPath>\n</dependency>\n"
sed -i "/<dependencies>/a ${dependencycommon}" $pomfile

# 4. compile source code
cd $HADOOP_SRC_DIR
mvn package -Pdist,native -DskipTests -Dmaven.javadoc.skip=true -DskipShade -e -Drequire.isal
