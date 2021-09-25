/*
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.apache.crail.storage.tcp;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;
import java.nio.file.Paths;
import java.util.concurrent.ConcurrentHashMap;

import org.apache.crail.conf.CrailConfiguration;
import org.apache.crail.conf.CrailConstants;
import org.apache.crail.storage.StorageResource;
import org.apache.crail.storage.StorageServer;
import org.apache.crail.storage.StorageUtils;
import org.apache.crail.utils.CrailUtils;
import org.slf4j.Logger;

import com.ibm.narpc.NaRPCServerChannel;
import com.ibm.narpc.NaRPCServerEndpoint;
import com.ibm.narpc.NaRPCServerGroup;
import com.ibm.narpc.NaRPCService;

public class HddStorageServer implements Runnable, StorageServer, NaRPCService<TcpStorageRequest, TcpStorageResponse> {
	private static final Logger LOG = CrailUtils.getLogger();
	
	private NaRPCServerGroup<TcpStorageRequest, TcpStorageResponse> serverGroup;
	private NaRPCServerEndpoint<TcpStorageRequest, TcpStorageResponse> serverEndpoint;
	private InetSocketAddress address;
	private boolean alive;
	private long regions;
	private long keys;
    // // it seems that Hdd does not need data Buffers
	private ConcurrentHashMap<Integer, ByteBuffer> dataBuffers;
	private String dataDirPath;
    
    // Xiaolu add
    private ConcurrentHashMap<Integer, String> pathMap;
    private ConcurrentHashMap<Integer, Long> startAddrMap;
	
	@Override
	public void init(CrailConfiguration conf, String[] args) throws Exception {
		TcpStorageConstants.init(conf, args);
		
		this.serverGroup = new NaRPCServerGroup<TcpStorageRequest, TcpStorageResponse>(this, TcpStorageConstants.STORAGE_TCP_QUEUE_DEPTH, (int) CrailConstants.BLOCK_SIZE*2, false, TcpStorageConstants.STORAGE_TCP_CORES);
		this.serverEndpoint = serverGroup.createServerEndpoint();
		this.address = StorageUtils.getDataNodeAddress(TcpStorageConstants.STORAGE_TCP_INTERFACE, TcpStorageConstants.STORAGE_HDD_PORT);
		serverEndpoint.bind(address);
		this.alive = false;
		this.regions = TcpStorageConstants.STORAGE_HDD_STORAGE_LIMIT/TcpStorageConstants.STORAGE_TCP_ALLOCATION_SIZE;
		this.keys = 0;
		this.dataBuffers = new ConcurrentHashMap<Integer, ByteBuffer>();
		this.dataDirPath = StorageUtils.getDatanodeDirectory(TcpStorageConstants.STORAGE_HDD_DATA_PATH, address);
		StorageUtils.clean(TcpStorageConstants.STORAGE_TCP_DATA_PATH, dataDirPath);

        // xiaolu add
        this.pathMap = new ConcurrentHashMap<Integer, String>();
        this.startAddrMap = new ConcurrentHashMap<Integer, Long>();
	}

	@Override
	public void printConf(Logger logger) {
		TcpStorageConstants.printConf(logger);
		System.out.printf("Using HDD Tier\n");
	}

    @Override
    public StorageResource allocateResource() throws Exception {
        StorageResource resource = null;
        if (keys < regions) {
            int fileId = (int) keys++;

			String dataFilePath = Paths.get(dataDirPath, Integer.toString(fileId)).toString();
            pathMap.put(fileId, dataFilePath);

            // Xiaolu hardcode a start address
            long address = 140068345872384l + (long)fileId * (long)TcpStorageConstants.STORAGE_TCP_ALLOCATION_SIZE;
            startAddrMap.put(fileId, address);

            resource = StorageResource.createResource(address, (int)TcpStorageConstants.STORAGE_TCP_ALLOCATION_SIZE, fileId);
        }
        return resource;
    }
	@Override
	public InetSocketAddress getAddress() {
		return address;
	}

	@Override
	public boolean isAlive() {
		return alive;
	}

	@Override
	public void run() {
		try {
			LOG.info("running Hdd storage server, address " + address);
			this.alive = true;
			while(true){
				NaRPCServerChannel endpoint = serverEndpoint.accept();
				LOG.info("Hdd new connection " + endpoint.address());
			}
		} catch(Exception e){
			e.printStackTrace();
		}
	}

	@Override
	public TcpStorageRequest createRequest() {
		return new TcpStorageRequest();
	}

    // XL add new processrequest
	@Override
	public TcpStorageResponse processRequest(TcpStorageRequest request) {
        LOG.info("XL-HddStorageServer::processRequest");
		if (request.type() == TcpStorageProtocol.REQ_WRITE){
            // get write request
			TcpStorageRequest.WriteRequest writeRequest = request.getWriteRequest();
            int key = writeRequest.getKey();
            long startAddr = startAddrMap.get(key);
            long offset = writeRequest.getAddress() - startAddr;
            LOG.info("XL-HddStorageServer::processRequest, key " + key + ", address " + writeRequest.getAddress() + ", length " + writeRequest.length() + ", remaining " + writeRequest.getBuffer().remaining() + ", offset " + offset);

            try {
                String path = pathMap.get(key);
	    		RandomAccessFile dataFile = new RandomAccessFile(path, "rws");
                dataFile.seek(offset);
                int len = writeRequest.length();
                byte[] databuf = new byte[len];
                writeRequest.getBuffer().get(databuf);
                dataFile.write(databuf);
                dataFile.close();
            } catch (IOException e) {
                e.printStackTrace();
            }

			TcpStorageResponse.WriteResponse writeResponse = new TcpStorageResponse.WriteResponse(writeRequest.length());
			return new TcpStorageResponse(writeResponse);
		} else if (request.type() == TcpStorageProtocol.REQ_READ){
			TcpStorageRequest.ReadRequest readRequest = request.getReadRequest();
            int key = readRequest.getKey();
            long startAddr = startAddrMap.get(key);
            long offset = readRequest.getAddress() - startAddr;
            LOG.info("XL-HddStorageServer::processRequest, key " + key + ", address " + readRequest.getAddress() + ", length " + readRequest.length() + ", offset " + offset);

            int len = readRequest.length();
            byte[] databuf = new byte[len];
            try{
                String path = pathMap.get(key);
                RandomAccessFile dataFile = new RandomAccessFile(path, "rws");
                dataFile.seek(offset);
                dataFile.read(databuf);
                dataFile.close();
            } catch (IOException e) {
                e.printStackTrace();
            }

            ByteBuffer buffer = ByteBuffer.wrap(databuf);

			TcpStorageResponse.ReadResponse readResponse = new TcpStorageResponse.ReadResponse(buffer);
			return new TcpStorageResponse(readResponse);
		} else {
			LOG.info("processing unknown request");
			return new TcpStorageResponse(TcpStorageProtocol.RET_RPC_UNKNOWN);
		}
	}

	@Override
	public void addEndpoint(NaRPCServerChannel channel){
	}

	@Override
	public void removeEndpoint(NaRPCServerChannel channel){
	}
}
