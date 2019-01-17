/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.apache.hadoop.hdfs.server.blockmanagement;

import java.util.*;

import org.apache.hadoop.fs.StorageType;
import org.apache.hadoop.hdfs.AddBlockFlag;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hdfs.protocol.Block;
import org.apache.hadoop.hdfs.protocol.BlockStoragePolicy;
import org.apache.hadoop.hdfs.server.blockmanagement.DatanodeDescriptor;
import org.apache.hadoop.net.NetworkTopology;
import org.apache.hadoop.net.Node;

import redis.clients.jedis.BinaryJedis;
import redis.clients.jedis.JedisPool;
import redis.clients.jedis.JedisPoolConfig;

import oec.protocol.CoorCommand;

public class BlockPlacementPolicyOEC extends BlockPlacementPolicyDefault {
  
  Map<String, String> obj2Loc = new HashMap<String, String>();
  JedisPoolConfig sendJedisPoolConfig;
  JedisPool sendJedisPool;
  JedisPoolConfig localJedisPoolConfig;
  JedisPool localJedisPool;

  @Override
  public void initialize(Configuration conf,  FSClusterStats stats,
                         NetworkTopology clusterMap,
                         Host2NodesMap host2datanodeMap) {
    super.initialize(conf, stats, clusterMap, host2datanodeMap);
    sendJedisPoolConfig = new JedisPoolConfig();
    sendJedisPoolConfig.setMaxTotal(1);
    sendJedisPool = new JedisPool(sendJedisPoolConfig, conf.get("oec.controller.addr"));
    localJedisPoolConfig = new JedisPoolConfig();
    localJedisPoolConfig.setMaxTotal(1);
    localJedisPool = new JedisPool(localJedisPoolConfig, "127.0.0.1");
  }
  
  public DatanodeStorageInfo[] chooseTarget(String src,
      int numOfReplicas,
      Node writer,
      Set<Node> excludedNodes,
      long blocksize,
      List<DatanodeDescriptor> favoredNodes,
      BlockStoragePolicy storagePolicy,
      EnumSet<AddBlockFlag> flags,
      Block b) {
    System.out.println("XL::BlockPlacementPolicyOEC.chooseTarget.for block " + b.getBlockName() + " of file " + src);
//    System.out.println("XL::BlockPlacementPolicyOEC.chooseTarget.writeIp == null ? " + (writer == null));
    String writeraddr = writer.getName();
//    System.out.println("XL::BlockPlacementPolicyOEC.chooseTarget.writer = " + writeraddr);
    int idx = writeraddr.indexOf(":");
    String writerIp;
    if (idx > 0) writerIp = writeraddr.substring(0, idx);
    else writerIp = writeraddr;
//    System.out.println("XL::BlockPlacementPolicyOEC.chooseTarget.clientIP = " + writer.getName());
    DatanodeStorageInfo[] toret = new DatanodeStorageInfo[numOfReplicas];

    String[] locs = null;
    String objname = src;

    // check whether use oec to get the location
    boolean linkOEC = conf.getBoolean("link.oec", false);
    if (linkOEC) {
      // now send request to OpenEC to get locs
      if (locs == null) {
        String oecCoorAddr = conf.get("oec.controller.addr");
        CoorCommand coorCmd = new CoorCommand();
        coorCmd.buildType1(1, writerIp, objname, numOfReplicas);
        coorCmd.sendTo(sendJedisPool);
        locs = coorCmd.waitForLocation(localJedisPool);
      }
      for (int i=0; i<numOfReplicas; i++) {
        System.out.println("XL::BlockPlacementPolicyOEC.chooseTarget.loc i = " + locs[i]);
        DatanodeDescriptor des = (DatanodeDescriptor)clusterMap.getLocByIp(locs[i]);
        System.out.println("XL::BlockPlacementPolicyOEC.chooseTarget.des == null ? " + (des == null));
        toret[i] = des.chooseStorage4Block(StorageType.DISK, blocksize);
      }
    } else {
      if (numOfReplicas == 1) {
        DatanodeDescriptor des = (DatanodeDescriptor)clusterMap.getLocRandom();
        toret[0] = des.chooseStorage4Block(StorageType.DISK, blocksize);
      } else {
        // local
        DatanodeDescriptor des = (DatanodeDescriptor)writer;
        toret[0] = des.chooseStorage4Block(StorageType.DISK, blocksize);
        for (int i=1; i<numOfReplicas; i++) {
          DatanodeDescriptor curdes = (DatanodeDescriptor)clusterMap.getLocRandom();
          toret[i] = des.chooseStorage4Block(StorageType.DISK, blocksize);
        }
      }
    } 
    System.out.println("XL::BlockPlacementPolicyOEC.chooseTarget.locs: ");
    for (int i=0; i<numOfReplicas; i++) {
      System.out.println(toret[i].getDatanodeDescriptor().getName());
    }

    return toret;
  }
}
