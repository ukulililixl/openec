#!/bin/bash
# usage: ./env.sh fstype
#     fstype: HDFS3

FSTYPE=$1
echo "evn.sh fstyp: "$FSTYPE

if [ "$FSTYPE" = "HDFS3" ]; then
  export CLASSPATH=`hadoop classpath --glob`
elif [ "$FSTYPE" = "HDFSRAID" ]; then
  export CLASSPATH=$JAVA_HOME/lib
  export CLASSPATH=${CLASSPATH}:${HADOOP_HOME}/conf
  export CLASSPATH=${CLASSPATH}:${JAVA_HOME}/lib/tools.jar
  export CLASSPATH=${CLASSPATH}:$HADOOP_HOME/build/classes
  export CLASSPATH=${CLASSPATH}:$HADOOP_HOME/build
  # add libs to CLASSPATH
  for f in $HADOOP_HOME/lib/*.jar; do
  CLASSPATH=${CLASSPATH}:$f;
  done
  
  for ff in $HADOOP_HOME/*.jar; do
  CLASSPATH=${CLASSPATH}:$ff
  done
  
  for f in $HADOOP_HOME/lib/jsp-2.0/*.jar; do
  CLASSPATH=${CLASSPATH}:$f;
  done
  
  if [ -d "$HADOOP_HOME/build/ivy/lib/Hadoop/common" ]; then
  for f in $HADOOP_HOME/build/ivy/lib/Hadoop/common/*.jar; do
  CLASSPATH=${CLASSPATH}:$f;
  done
  fi
  # LD
  export LD_LIBRARY_PATH=${HADOOP_HOME}/build/c++/Linux-amd64-64/lib/:${JAVA_HOME}/jre/lib/amd64/server/:${JAVA_HOME}/jre/lib/amd64/:${HADOOP_HOME}/build/native/Linux-amd64-64/lib
fi
