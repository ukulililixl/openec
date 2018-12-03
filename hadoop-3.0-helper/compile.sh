#!/bin/bash

javac -cp .:./commons-pool2-2.4.2.jar:./commons-pool2-2.4.2-javadoc.jar:./jedis-3.0.0-SNAPSHOT.jar oec/protocol/AGCommand.java oec/protocol/CoorCommand.java
jar cf oec-fs-protocol.jar oec/protocol/AGCommand.class oec/protocol/CoorCommand.class

#copy to hadoop-3.0 directory
DIR=/home/xiaolu/OpenEC/hadoop-3.0.0-src/oeclib
cp oec-fs-protocol.jar $DIR/

# #copy to java client directory
# JAVACLIENTDIR=/home/xiaolu/OpenEC/OpenEC/hadoop-3.0-helper/client
# cp oec-fs-protocol.jar $JAVACLIENTDIR/
