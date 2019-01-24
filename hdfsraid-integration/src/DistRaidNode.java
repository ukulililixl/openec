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

package org.apache.hadoop.raid;

import java.io.IOException;
import java.util.List;
import java.util.LinkedList;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.hadoop.conf.Configuration;

import org.apache.hadoop.util.Daemon;

import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.FileStatus;

import org.apache.hadoop.raid.DistRaid.EncodingCandidate;
import org.apache.hadoop.raid.protocol.PolicyInfo;

/**
 * Implementation of {@link RaidNode} that uses map reduce jobs to raid files.
 */
public class DistRaidNode extends RaidNode {

  public static final Log LOG = LogFactory.getLog(DistRaidNode.class);

  /** Daemon thread to monitor raid job progress */
  JobMonitor jobMonitor = null;
  Daemon jobMonitorThread = null;

  public DistRaidNode(Configuration conf) throws IOException {
    super(conf);
    this.jobMonitor = new JobMonitor(conf);
    this.jobMonitorThread = new Daemon(this.jobMonitor);
    this.jobMonitorThread.start();
    
    LOG.info("created");
  }

  /**
   * {@inheritDocs}
   */
  @Override
  public void join() {
    super.join();
    try {
      if (jobMonitorThread != null) jobMonitorThread.join();
    } catch (InterruptedException ie) {
      // do nothing
    }
  }
  
  /**
   * {@inheritDocs}
   */
  @Override
  public void stop() {
    if (stopRequested) {
      return;
    }
    super.stop();
    if (jobMonitor != null) jobMonitor.running = false;
    if (jobMonitorThread != null) jobMonitorThread.interrupt();
  }


  static List<DistRaid> raidList = new LinkedList<DistRaid> ();
  /**
   * {@inheritDocs}
   */
  @Override
  void raidFiles(PolicyInfo info, List<FileStatus> paths) throws IOException {
    if (info == null) {
      LOG.info("DistRaidNode.check time 92");
      boolean finish;
      while (raidList.size() > 0) {
        long startTime = raidList.get(0).getStartTime();
        finish = raidList.get(0).checkComplete();
        for (int i=1; i<raidList.size(); i++) {
          DistRaid curDR = raidList.get(i);
          finish = finish & curDR.checkComplete();
          if (curDR.getStartTime() < startTime) startTime = curDR.getStartTime();
          if (!finish) break;
        }
        if (finish) {
          long endTime = System.currentTimeMillis();
          LOG.info("DistRaidNode.encodeTime = " + (endTime - startTime));
          raidList.clear();
          break;
        }  
      }
    } else {
      raidFiles(conf, jobMonitor, paths, info);
    }
  }

  final static DistRaid raidFiles(Configuration conf, JobMonitor jobMonitor,
      List<FileStatus> paths, PolicyInfo info) throws IOException {
    LOG.info("DistRaid.raidFiles 99");
    List<EncodingCandidate> lec = splitPaths(conf,
        Codec.getCodec(info.getCodecId()), paths);
    LOG.info("DistRaid.raidFiles 102.EncodingCandidate.size = " + lec.size());
    
    // We already checked that no job for this policy is running
    // So we can start a new job.
    DistRaid dr = new DistRaid(conf);
    //add paths for distributed raiding
    dr.addRaidPaths(info, lec);
    boolean started = dr.startDistRaid();
    if (started) {
      jobMonitor.monitorJob(info.getName(), dr);
      raidList.add(dr);
    } else {
      return null;
    }
    return dr;
  }
  /**
   * {@inheritDocs}
   */
  @Override
  int getRunningJobsForPolicy(String policyName) {
    return jobMonitor.runningJobsCount(policyName);
  }

  @Override
  public String raidJobsHtmlTable(JobMonitor.STATUS st) {
    return jobMonitor.toHtml(st);
  }
}
