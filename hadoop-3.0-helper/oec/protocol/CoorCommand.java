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
package oec.protocol;

import java.nio.ByteBuffer;
import java.util.*;

import redis.clients.jedis.BinaryJedis;
import redis.clients.jedis.JedisPool;
import redis.clients.jedis.JedisPoolConfig;
import redis.clients.jedis.Pipeline;
import redis.clients.jedis.Response;

/*
 * OECCoordinator Command format
 * coor_request: type
 *    type=0 (client update file with ecid)| file name len| file name | ecid len | ecid | 
 *    type=1 (client request a location)| clientip | objname |
 */

public class CoorCommand {
  public byte[] coorCmd;
  int cmLen;

  public int type;
  
  // for type 0
  String filename;
  String ecid;

  // for type 1
  String requestIp;
  int numOfReplicas;

  // for type 6

  public CoorCommand() {
    this.coorCmd = new byte[1024];
    this.cmLen = 0;
  }

  public void buildType0(int type, String filename, String ecid) {
    // set up corresponding parameters
    this.type = type;
    this.filename = filename;
    this.ecid = ecid;

    // fill in coorCmd
    // 1. type
    byte[] typebytes = intToBytes(this.type);
    System.arraycopy(typebytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;

    // 2. filename
    // 2.1 len of filename
    byte[] filenamelenbytes = intToBytes(this.filename.length());
    System.arraycopy(filenamelenbytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;
    // 2.2 filename
    System.arraycopy(this.filename.getBytes(), 0, this.coorCmd, this.cmLen, this.filename.length()); this.cmLen += this.filename.length();
    
    // 3. ecid
    // 3.1 len of ecid
    byte[] ecidlenbytes = intToBytes(this.ecid.length());
    System.arraycopy(ecidlenbytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;
    // 3.2 ecid
    System.arraycopy(this.ecid.getBytes(), 0, this.coorCmd, this.cmLen, this.ecid.length()); this.cmLen += this.ecid.length();
  }

  public void buildType1(int type, String clientip, String filename, int numOfReplicas) {
    // set up corresponding parameters
    this.type = type;
    this.requestIp = clientip;
    this.filename = filename;
    this.numOfReplicas = numOfReplicas;
//    this.objIdx = idx;

    // 1. type
    byte[] typebytes = intToBytes(this.type);
    System.arraycopy(typebytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;

    // 2. clientip
    int ip = ipStr2Int(this.requestIp);
    byte[] ipbytes = intToBytes(ip);
    System.arraycopy(ipbytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;

    // 3. filename
    // 2.1 len of filename
    byte[] filenamelenbytes = intToBytes(this.filename.length());
    System.arraycopy(filenamelenbytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;
    // 2.2 filename
    System.arraycopy(this.filename.getBytes(), 0, this.coorCmd, this.cmLen, this.filename.length()); this.cmLen += this.filename.length();

    // 4. numOfReplicas
    byte[] numrepbytes = intToBytes(this.numOfReplicas);
    System.arraycopy(numrepbytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;

//    // 4. objIdx
//    byte[] idxbytes = intToBytes(this.objIdx);
//    System.arraycopy(idxbytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;
  }

  public void buildType6(int type, String clientIp, String objname) {
    // set up corresponding parameters
    this.type = type;
    this.filename = objname;
    this.requestIp = clientIp;

    // 1. type
    byte[] typebytes = intToBytes(this.type);
    System.arraycopy(typebytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;

    // 2. clientip
    int ip = ipStr2Int(this.requestIp);
    byte[] ipbytes = intToBytes(ip);
    System.arraycopy(ipbytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;

    // 2. filename
    // 2.1 len of filename
     byte[] filenamelenbytes = intToBytes(this.filename.length());
    System.arraycopy(filenamelenbytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;
    // 2.2 filename
    System.arraycopy(this.filename.getBytes(), 0, this.coorCmd, this.cmLen, this.filename.length()); this.cmLen += this.filename.length();
  }

   public void buildType11(int type, String clientIp, String objname) {
    // set up corresponding parameters
    this.type = type;
    this.filename = objname;
    this.requestIp = clientIp;

    // 1. type
    byte[] typebytes = intToBytes(this.type);
    System.arraycopy(typebytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;

    // 2. clientip
    int ip = ipStr2Int(this.requestIp);
    byte[] ipbytes = intToBytes(ip);
    System.arraycopy(ipbytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;

    // 2. filename
    // 2.1 len of filename
     byte[] filenamelenbytes = intToBytes(this.filename.length());
    System.arraycopy(filenamelenbytes, 0, this.coorCmd, this.cmLen, 4); this.cmLen += 4;
    // 2.2 filename
    System.arraycopy(this.filename.getBytes(), 0, this.coorCmd, this.cmLen, this.filename.length()); this.cmLen += this.filename.length();
  }

  public void sendTo(String ip) {
    JedisPoolConfig sendJedisPoolConfig = new JedisPoolConfig();
    sendJedisPoolConfig.setMaxTotal(1);
    JedisPool sendJedisPool = new JedisPool(sendJedisPoolConfig, ip);

    try (BinaryJedis jedis = sendJedisPool.getResource()) {
      jedis.rpush("coor_request".getBytes(), this.coorCmd);
    } catch (Exception i) {
      i.printStackTrace();
      System.exit(1);
    }
  }

  public void sendTo(JedisPool sendJedisPool) {
    try (BinaryJedis jedis = sendJedisPool.getResource()) {
      jedis.rpush("coor_request".getBytes(), this.coorCmd);
    } catch (Exception i) {
      i.printStackTrace();
      System.exit(1);
    }
  }

  public String waitForLocation(String localIp) {
    JedisPoolConfig localJedisPoolConfig = new JedisPoolConfig();
    localJedisPoolConfig.setMaxTotal(1);
    JedisPool localJedisPool = new JedisPool(localJedisPoolConfig, localIp);

    try (BinaryJedis jedis = localJedisPool.getResource()) {
//      String key = "loc:"+this.filename+":"+objIdx;
      String key = "loc:"+this.filename;
      List<byte[]> l = jedis.blpop(0, key.getBytes());
      byte[] loc = l.get(1);
      String location = getIPAddressFromBytes(loc);
      return location;
    } catch (Exception i) {
      System.out.println("Exception wait for location");
      return null;
    }
  }

  public String[] waitForLocation(JedisPool localJedisPool) {
    String[] toret = new String[this.numOfReplicas];
    try (BinaryJedis jedis = localJedisPool.getResource()) {
      String key = "loc:"+this.filename;
      List<byte[]> l = jedis.blpop(0, key.getBytes());
      byte[] locs = l.get(1);
      int offset=0;
      for (int i=0; i<this.numOfReplicas; i++) {
        byte[] curbytes = new byte[4];
        System.arraycopy(locs, i*4, curbytes, 0, 4);
        toret[i] = getIPAddressFromBytes(curbytes);
      }
      return toret;
    } catch (Exception i) {
      System.out.println("Exception wait for location");
      return null;
    }
  }

  public static String getIPAddressFromBytes(byte[] ip) {
    String toret="";
    for (int i=0; i<4; i++) {
      int t = 0xFF & ip[i];
      toret+="." + t;
    }
    return toret.substring(1);
  }

  public static byte[] intToBytes(int val) {
    ByteBuffer bb = ByteBuffer.allocate(4);
    bb.putInt(val);
    return bb.array();
  }

  public static int ipStr2Int(String ip) {
    int result = 0;
    for(String part : ip.split("\\.")) {
      result = result << 8;
      result |= Integer.parseInt(part);
    }
    return result;
  }
}
