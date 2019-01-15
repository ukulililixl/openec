#!/bin/bash

HADOOP_SRC_DIR=/home/xiaolu/OpenEC/hadoop-3.0.0-src

# 0. download third party libraries from our website here
# wget commons-pool2-2.4.2.jar
# wget commons-pool2-2.4.2-javadoc.jar
# wget jedis-3.0.0-SNAPSHOT.jar

# 1. compile java version of openec command
#javac -cp .:./commons-pool2-2.4.2.jar:./commons-pool2-2.4.2-javadoc.jar:./jedis-3.0.0-SNAPSHOT.jar oec/protocol/AGCommand.java oec/protocol/CoorCommand.java
#jar cf oec-fs-protocol.jar oec/protocol/AGCommand.class oec/protocol/CoorCommand.class

# 2. copy libraries to hadoop src 
# mkdir $HADOOP_SRC_DIR/oeclib
# cp commons-pool2-2.4.2.jar $HADOOP_SRC_DIR/oeclib
# cp commons-pool2-2.4.2-javadoc.jar $HADOOP_SRC_DIR/oeclib
# cp jedis-3.0.0-SNAPSHOT.jar $HADOOP_SRC_DIR/oeclib
# cp oec-fs-protocol.jar $HADOOP_SRC_DIR/oeclib

# 3. copy openec modifications to hadoop src
# cp src/BlockManager.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/
# cp src/BlockPlacementPolicyDefault.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/
# cp src/BlockPlacementPolicy.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/
# cp src/BlockPlacementPolicyOEC.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/
# cp src/CommandWithDestination.java $HADOOP_SRC_DIR/hadoop-common-project/hadoop-common/src/main/java/org/apache/hadoop/fs/shell/
# cp src/FSDirWriteFileOp.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/namenode/
# cp src/FSNamesystem.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/namenode/
# cp src/LowRedundancyBlocks.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/blockmanagement/
# cp src/NameNodeRpcServer.java $HADOOP_SRC_DIR/hadoop-hdfs-project/hadoop-hdfs/src/main/java/org/apache/hadoop/hdfs/server/namenode/
# cp src/NetworkTopology.java $HADOOP_SRC_DIR/hadoop-common-project/hadoop-common/src/main/java/org/apache/hadoop/net/

# 4. compile source code
# cd $HADOOP_SRC_DIR
