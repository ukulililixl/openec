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
 * OECAgent Command format
 * agent_request: type
 *    type=0 (client write data)| filename | k | m | mode |
 */

public class AGCommand {

  public byte[] agCmd;
  int cmLen;

  public int type; 

  // for type 0 (client write data request to OECAgent)
  String filename;
  String ecid;
  String mode;
  int filesizeMB;

  // for type 1
  // filename
  
  public AGCommand() {
    this.agCmd = new byte[1024];   // xiaolu hardcode here
    this.cmLen = 0;
  } 
  
  public void buildType0(int type, String filename, String ecid, String mode, int filesizeMB) {
    // set up corresponding parameters
    this.type = type;
    this.filename = filename;
    this.ecid = ecid;
    this.mode = mode;
    this.filesizeMB = filesizeMB;

    // fill in agCmd
    // 1. type
    byte[] typebytes = intToBytes(this.type);
    System.arraycopy(typebytes, 0, this.agCmd, this.cmLen, 4); this.cmLen += 4;

    // 2. filename
    // 2.1 len of filename
    byte[] filenamelenbytes = intToBytes(this.filename.length());
    System.arraycopy(filenamelenbytes, 0, this.agCmd, this.cmLen, 4); this.cmLen += 4;
    // 2.2 filename bytes
    System.arraycopy(this.filename.getBytes(), 0, this.agCmd, this.cmLen, this.filename.length()); this.cmLen += this.filename.length();

    // 3. ecid
    // 3.1 lenof ecid
    byte[] ecidlenbytes = intToBytes(this.ecid.length());
    System.arraycopy(ecidlenbytes, 0, this.agCmd, this.cmLen, 4); this.cmLen += 4;
    // 3.2 ecid bytes
    System.arraycopy(this.ecid.getBytes(), 0, this.agCmd, this.cmLen, this.ecid.length()); this.cmLen += this.ecid.length();

    // 4. mode
    // 4.1 mode len
    byte[] modelenbytes = intToBytes(this.mode.length());
    System.arraycopy(modelenbytes, 0, this.agCmd, this.cmLen, 4); this.cmLen += 4;
    // 4.2 mode bytes
    System.arraycopy(this.mode.getBytes(), 0, this.agCmd, this.cmLen, this.mode.length()); this.cmLen += this.mode.length();

    // 5. filesizeMB
    byte[] filesizeMBbytes = intToBytes(filesizeMB);
    System.arraycopy(filesizeMBbytes, 0, this.agCmd, this.cmLen, 4); this.cmLen += 4;
  }

  public void buildType1(int type, String filename) {
    // set up corresponding parameters
    this.type = type;
    this.filename = filename;

    // fill in agCmd
    // 1. type
    byte[] typebytes = intToBytes(this.type);
    System.arraycopy(typebytes, 0, this.agCmd, this.cmLen, 4); this.cmLen += 4;

    // 2. filename
    // 2.1 len of filename
    byte[] filenamelenbytes = intToBytes(this.filename.length());
    System.arraycopy(filenamelenbytes, 0, this.agCmd, this.cmLen, 4); this.cmLen += 4;
    // 2.2 filename bytes
    System.arraycopy(this.filename.getBytes(), 0, this.agCmd, this.cmLen, this.filename.length()); this.cmLen += this.filename.length();
  }

  public void sendTo(String ip) {
    JedisPoolConfig sendJedisPoolConfig = new JedisPoolConfig();
    sendJedisPoolConfig.setMaxTotal(1);
    JedisPool sendJedisPool = new JedisPool(sendJedisPoolConfig, ip);

    try (BinaryJedis jedis = sendJedisPool.getResource()) {
      jedis.rpush("ag_request".getBytes(), this.agCmd);
    } catch (Exception i) {
      i.printStackTrace();
      System.exit(1);
    }
  }

  public static byte[] intToBytes(int val) {
    ByteBuffer bb = ByteBuffer.allocate(4);
    bb.putInt(val);
    return bb.array();
  }
}
