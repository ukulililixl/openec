------------------ README ------------------------

OpenEC is a new erasure coding framework which provides 
unified and configurable erasure coding management for
distributed storage systems. Please refer to our paper
published in FAST 19 for details.

Now OpenEC supports to run atop Hadoop 3 HDFS, HDFS-RAID
and Quantcast File System. 

Build for Hadoop3 HDFS:

$> cd OpenEC-v1.0; cmake . -DFS\_TYPE:STRING=HDFS3; make

Build for HDFS-RAID:

$> cd OpenEC-v1.0; cmake . -DFS\_TYPE:STRING=HDFSRAID; make

Build for QFS:

$> cd OpenEC-v1.0; cmake . -DFS\_TYPE:STRING=QFS; make
