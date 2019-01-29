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
package org.apache.hadoop.hdfs.server.namenode;

import java.io.IOException;
import java.util.List;
import java.util.LinkedList;
import java.util.HashMap;
import java.util.Random;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.net.DNSToSwitchMapping;
import org.apache.hadoop.net.NetworkTopology;
import org.apache.hadoop.net.Node;
import org.apache.hadoop.raid.Codec;
import org.apache.hadoop.util.HostsFileReader;

import redis.clients.jedis.BinaryJedis;
import redis.clients.jedis.JedisPool;
import redis.clients.jedis.JedisPoolConfig;

import oec.protocol.CoorCommand;

/**
 * This BlockPlacementPolicy uses a simple heuristic, random placement of
 * the replicas of a newly-created block for all the files in the system, 
 * for the purpose of spreading out the 
 * group of blocks which used by RAID for recovering each other. 
 * This is important for the availability of the blocks. 
 * 
 * Replication of an existing block continues to use the default placement
 * policy.
 * 
 * This simple block placement policy does not guarantee that
 * blocks on the RAID stripe are on different nodes. However, BlockMonitor
 * will periodically scans the raided files and will fix the placement
 * if it detects violation. 
 */

public class BlockPlacementPolicyOEC extends BlockPlacementPolicyRaid {

  Configuration conf;
  JedisPoolConfig sendJedisPoolConfig;
  JedisPool sendJedisPool;
  JedisPoolConfig localJedisPoolConfig;
  JedisPool localJedisPool;
  HashMap<String, LinkedList<DatanodeDescriptor>> stripeInfo;
  String repairIp;
  
  @Override 
  public void initialize(Configuration conf,  FSClusterStats stats,
      NetworkTopology clusterMap, HostsFileReader hostsReader,
	  DNSToSwitchMapping dnsToSwitchMapping, FSNamesystem namesystem) {
	super.initialize(conf, stats, clusterMap,
	    hostsReader, dnsToSwitchMapping, namesystem);
    this.conf = conf;
    sendJedisPoolConfig = new JedisPoolConfig();
    sendJedisPoolConfig.setMaxTotal(1);
    sendJedisPool = new JedisPool(sendJedisPoolConfig, conf.get("oec.controller.addr"));
    localJedisPoolConfig = new JedisPoolConfig();
    localJedisPoolConfig.setMaxTotal(1);
    localJedisPool = new JedisPool(localJedisPoolConfig, "127.0.0.1");
    stripeInfo = new HashMap<String, LinkedList<DatanodeDescriptor>>();
    this.repairIp = "127.0.0.1";
  }

//  @Override
//  protected FileInfo getFileInfo(FSInodeInfo srcINode, String path) throws IOException {
//    FileInfo info = super.getFileInfo(srcINode, path);
//    if (info.type == FileType.NOT_RAID) {
//      return new FileInfo(FileType.SOURCE, Codec.getCodec("rs"));
//    }
//    return info;
//  }

  public DatanodeDescriptor[] chooseTarget(String srcInode,
      	  int numOfReplicas,
      	  DatanodeDescriptor writer,
      	  List<DatanodeDescriptor> chosenNodes,
      	  List<Node> excludesNodes,
      	  long blocksize,
          boolean useOEC){
    String writerIp;
    if (writer != null) writerIp = writer.getHostName();
    else writerIp = this.repairIp; 

    DatanodeDescriptor[] toret = new DatanodeDescriptor[numOfReplicas];

    boolean linkOEC = conf.getBoolean("link.oec", false);
    System.out.println("linkOEC = " + linkOEC);

    String[] locs = null;
    String objname = srcInode;

    if (linkOEC) {
      // send request to openec to get location
      if (locs == null) {
        String oecCoorAddr = conf.get("oec.controller.addr");
        CoorCommand coorCmd = new CoorCommand();
        coorCmd.buildType1(1, writerIp, objname, numOfReplicas);
        coorCmd.sendTo(sendJedisPool);
        locs = coorCmd.waitForLocation(localJedisPool);
      }

      System.out.println("XL::BlockPlacementPolicyOEC.chooseTarget.locs: ");
      for (int i=0; i<numOfReplicas; i++) {
        System.out.println(locs[i]);
        toret[i] = (DatanodeDescriptor)clusterMap.getLocByIp(locs[i]);
      }
    } else {
      if (numOfReplicas == 1) toret[0] = (DatanodeDescriptor)clusterMap.getLocRandom();
      else {
        if (writer != null) toret[0] = writer;
        else toret[0] = (DatanodeDescriptor)clusterMap.getLocRandom();
        for (int i=1; i<numOfReplicas; i++) {
          toret[i] = (DatanodeDescriptor)clusterMap.getLocRandom();
        }
      }
    }
    return toret;
  }

  @Override
  public DatanodeDescriptor[] chooseTarget(String srcInode,
		  int numOfReplicas,
		  DatanodeDescriptor writer,
		  List<DatanodeDescriptor> chosenNodes,
		  List<Node> excludesNodes,
		  long blocksize) {
    System.out.println("XL::BlockPlacementPolicyOEC.src = " + srcInode);
    DatanodeDescriptor[] toret;
    if (srcInode.contains("/benchmarks/IOTest/io_control/") ||
        srcInode.contains("oecobj") ||
        srcInode.contains("oecstripe") ||
        srcInode.contains("placetest") ||
        srcInode.contains("offlinepool")) {
      return chooseTarget(srcInode, numOfReplicas, writer, chosenNodes, excludesNodes, blocksize, true);
    } else {
      toret = super.chooseTarget(srcInode, numOfReplicas, writer, chosenNodes, excludesNodes, blocksize);

      if (srcInode.contains("tmp")) {
        // check in stripeInfo map whether there is corresponding source file
        String filename="default";
        boolean find = false;
        for (String key: this.stripeInfo.keySet()) {
          if (srcInode.contains(key)) {
            find = true;
            filename = key;
            break;
          }
        }
        if (find) {
          System.out.println("find corresponding stripe " + filename);
          // find out corresponding node ip; 
          LinkedList<DatanodeDescriptor> nodelist = this.stripeInfo.get(filename);
          LinkedList<String> ipList = new LinkedList<String>();
          for (int i=0; i<nodelist.size(); i++) {
            DatanodeDescriptor curnode = nodelist.get(i);
            String nodename = curnode.getName();
            int idx = nodename.indexOf(":");
            if (idx > 0) ipList.add(nodename.substring(0, idx));
          }
          System.out.println("list:");
          for (int i=0; i<ipList.size(); i++) System.out.println(ipList.get(i));
          List<String> candidates = clusterMap.getIpCandidates(ipList);
          Random randomGenerator = new Random();
          int randomidx = randomGenerator.nextInt(candidates.size());
          String chooseIpStr = candidates.get(randomidx);
          toret[0] = (DatanodeDescriptor)clusterMap.getLocByIp(chooseIpStr);
          System.out.println("chooseIp:"+chooseIpStr);
          
          nodelist.add(toret[0]);
          this.stripeInfo.put(filename, nodelist);
        } else {
          System.out.println("not find corresponding stripe");
          toret = super.chooseTarget(srcInode, numOfReplicas, writer, chosenNodes, excludesNodes, blocksize);
        }
      } else if (srcInode.contains("raidTest") ||
          srcInode.contains("recovery")) {
        int pos = srcInode.lastIndexOf("/");
	String filename = srcInode.substring(0,pos);
        pos = filename.lastIndexOf("/");
        filename = filename.substring(pos+1);
        System.out.println("filename = " + filename);
        LinkedList<DatanodeDescriptor> nodelist;
        if (!this.stripeInfo.containsKey(filename)) nodelist = new LinkedList<DatanodeDescriptor>();
        else nodelist = this.stripeInfo.get(filename);
        System.out.println("nodelist.size = " + nodelist.size());

        LinkedList<String> ipList = new LinkedList<String>();
        for (int i=0; i<nodelist.size(); i++) {
          DatanodeDescriptor curnode = nodelist.get(i);
          String nodename = curnode.getName();
          int idx = nodename.indexOf(":");
          if (idx > 0) ipList.add(nodename.substring(0, idx));
        }
        System.out.println("list:");
        for (int i=0; i<ipList.size(); i++) System.out.println(ipList.get(i));
        List<String> candidates = clusterMap.getIpCandidates(ipList);
        System.out.println("candidates:");
        for (int i=0; i<candidates.size(); i++) System.out.println(candidates.get(i));
        String chooseIpStr;
        if (srcInode.contains("recovery")) {
          if (candidates.contains(this.repairIp)) {
            chooseIpStr = this.repairIp;
          } else {
            Random randomGenerator = new Random();
            int randomidx = randomGenerator.nextInt(candidates.size());
            chooseIpStr = candidates.get(randomidx);
          }
        } else {
          Random randomGenerator = new Random();
          int randomidx = randomGenerator.nextInt(candidates.size());
          chooseIpStr = candidates.get(randomidx);
        }
        toret[0] = (DatanodeDescriptor)clusterMap.getLocByIp(chooseIpStr);
        System.out.println("chooseIp:"+chooseIpStr);

        nodelist.add(toret[0]);
        this.stripeInfo.put(filename, nodelist);
      }
      return toret;
    }
  }
}
