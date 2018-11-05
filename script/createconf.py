# usage:
#     python createconf.py pktsize control data avoidLocal repair
#         1. pktsize(131072)
#         2. control policy (random/balance)
#         3. data policy (random/balance)
#         4. avoidLocal (true/false)
#	  5. repair policy (random/balance)
#	  6. fstype(hadoop3/hadoop20)
#	  7. encode scheduling(delay/normal)
#	  8. encode policy (random/balance)
#	  9. repair scheduling (delay/normal)


import os
import subprocess
import sys

PACKETSIZE=sys.argv[1]
CONTROLP=sys.argv[2]
DATAP=sys.argv[3]
AVOIDLOCAL=sys.argv[4]
REPAIRP=sys.argv[5]
FSTYPE=sys.argv[6]
ENCODES=sys.argv[7]
ENCODEP=sys.argv[8]
REPAIRS=sys.argv[9]

cluster=[
"192.168.0.12",
"192.168.0.13",
"192.168.0.14",
"192.168.0.15",
"192.168.0.16",
"192.168.0.17",
"192.168.0.18",
"192.168.0.19"
]

networkMap={
"192.168.0.12":"/rack1",
"192.168.0.13":"/rack1",
"192.168.0.14":"/rack1",
"192.168.0.15":"/rack1",
"192.168.0.16":"/rack1",
"192.168.0.17":"/rack1",
"192.168.0.18":"/rack1",
"192.168.0.19":"/rack1"
}

for node in cluster:
    print node
    attr=[]
    
    line="<setting>\n"
    attr.append(line)

    line="<attribute><name>coor.address</name><value>192.168.0.12</value></attribute>\n"
    attr.append(line)

    line="<attribute><name>agents.address</name>\n"
    attr.append(line)

    for slave in cluster:
        if (slave == "192.168.0.12"):
             continue
        rack=networkMap[slave]
        line="<value>"+rack+"/"+slave+"</value>\n"
        attr.append(line)

    line="</attribute>\n"
    attr.append(line)

    line="<attribute><name>oec.coor.thread.num</name><value>10</value></attribute>\n"
    attr.append(line)
    
    line="<attribute><name>oec.agent.thread.num</name><value>10</value></attribute>\n"
    attr.append(line)

    line="<attribute><name>oec.cmddist.thread.num</name><value>10</value></attribute>\n"
    attr.append(line)
    
    line="<attribute><name>local.ip.address</name><value>"+node+"</value></attribute>\n"
    attr.append(line)
    
    line="<attribute><name>packet.size</name><value>"+PACKETSIZE+"</value></attribute>\n"
    attr.append(line)

    line="<attribute><name>underline.fs.type</name><value>"+FSTYPE+"</value></attribute>\n"
    attr.append(line)

    line="<attribute><name>fs.factory</name>\n"
    attr.append(line)
    line="<value><fstype>hadoop3</fstype><param>192.168.0.12,9000</param></value>\n"
    attr.append(line)
    line="<value><fstype>hadoop20</fstype><param>192.168.0.12,9000</param></value>\n"
    attr.append(line)
    line="</attribute>\n"
    attr.append(line)

#    line="<attribute><name>underline.fs.address</name><value>192.168.0.33:9000</value></attribute>\n"
#    attr.append(line)

    line="<attribute><name>control.policy</name><value>"+CONTROLP+"</value></attribute>\n"
    attr.append(line)

    line="<attribute><name>data.policy</name><value>"+DATAP+"</value></attribute>\n"
    attr.append(line)

    line="<attribute><name>encode.scheduling</name><value>"+ENCODES+"</value></attribute>\n"
    attr.append(line)

    line="<attribute><name>encode.policy</name><value>"+ENCODEP+"</value></attribute>\n"
    attr.append(line)

    line="<attribute><name>repair.scheduling</name><value>"+REPAIRS+"</value></attribute>\n"
    attr.append(line)

    line="<attribute><name>repair.policy</name><value>"+REPAIRP+"</value></attribute>\n"
    attr.append(line)

    line="<attribute><name>repair.threshold</name><value>10</value></attribute>\n"
    attr.append(line) 

    line="<attribute><name>placetest.avoidlocal</name><value>"+AVOIDLOCAL+"</value></attribute>\n"
    attr.append(line)

    line="<attribute><name>ec.policy</name>\n"
    attr.append(line)
    line="<value><id>rs_6_4</id><class>RSCONV</class><n>6</n><k>4</k><w>1</w><locality>false</locality></value>\n"
    attr.append(line)
#    line="<value><id>rs_9_6</id><class>RSCONV</class><n>9</n><k>6</k><cps>1</cps><locality>false</locality></value>\n"
#    attr.append(line)
#    line="<value><id>rs_12_8</id><class>RSCONV</class><n>12</n><k>8</k><cps>1</cps><locality>false</locality></value>\n"
#    attr.append(line)
#    line="<value><id>waslrc_6_2_2</id><class>WASLRC</class><n>10</n><k>6</k><cps>1</cps><locality>false</locality><param>2,2</param></value>\n"
#    attr.append(line)
#    line="<value><id>drc963</id><class>DRC963</class><n>9</n><k>6</k><cps>3</cps><locality>true</locality><param>3</param></value>\n"
#    attr.append(line)
#    line="<value><id>rsppr_9_6</id><class>RSPPR</class><n>9</n><k>6</k><cps>1</cps><locality>true</locality></value>\n"
#    attr.append(line)
#    line="<value><id>rspipe_9_6</id><class>RSPIPE</class><n>9</n><k>6</k><cps>1</cps><locality>false</locality></value>\n"
#    attr.append(line)
#    line="<value><id>rs_6_4</id><class>RSCONV</class><n>6</n><k>4</k><cps>1</cps><locality>false</locality></value>\n"
#    attr.append(line)
#    line="<value><id>ia_8_4</id><class>IA</class><n>8</n><k>4</k><cps>4</cps><locality>false</locality></value>\n"
#    attr.append(line)
#    line="<value><id>drc643</id><class>DRC643</class><n>6</n><k>4</k><cps>2</cps><locality>true</locality><param>3</param></value>\n"
#    attr.append(line)
#    line="<value><id>rsppr_6_4</id><class>RSPPR</class><n>6</n><k>4</k><cps>1</cps><locality>true</locality></value>\n"
#    attr.append(line)
#    line="<value><id>rspipe_6_4</id><class>RSPIPE</class><n>6</n><k>4</k><cps>1</cps><locality>false</locality></value>\n"
#    attr.append(line)
#    line="<value><id>rawrs</id><class>RAWRS</class><n>10</n><k>8</k><cps>1</cps><locality>false</locality><opt>2</opt></value>\n"
#    attr.append(line)
#    line="<value><id>clay_6_4</id><class>CLAY</class><n>6</n><k>4</k><cps>8</cps><locality>false</locality></value>\n"
#    attr.append(line)

    line="</attribute>\n"
    attr.append(line)

    line="<attribute><name>offline.pool</name>\n"
    attr.append(line)
    line="<value><poolid>rs_9_6_pool</poolid><ecid>rs_9_6</ecid></value>\n"
    attr.append(line)
    line="<value><poolid>rs_12_8_pool</poolid><ecid>rs_12_8</ecid></value>\n"
    attr.append(line)
    line="<value><poolid>waslrc_6_2_2_pool</poolid><ecid>waslrc_6_2_2</ecid></value>\n"
    attr.append(line)
    line="<value><poolid>drc963_pool</poolid><ecid>drc963</ecid></value>\n"
    attr.append(line)
    line="<value><poolid>rsppr_9_6_pool</poolid><ecid>rsppr_9_6</ecid></value>\n"
    attr.append(line)
    line="<value><poolid>rspipe_9_6_pool</poolid><ecid>rspipe_9_6</ecid></value>\n"
    attr.append(line)
    line="<value><poolid>rs_6_4_pool</poolid><ecid>rs_6_4</ecid></value>\n"
    attr.append(line)
    line="<value><poolid>ia_8_4_pool</poolid><ecid>ia_8_4</ecid></value>\n"
    attr.append(line)
    line="<value><poolid>drc643_pool</poolid><ecid>drc643</ecid></value>\n"
    attr.append(line)
    line="<value><poolid>rsppr_6_4_pool</poolid><ecid>rsppr_6_4</ecid></value>\n"
    attr.append(line)
    line="<value><poolid>rspipe_6_4_pool</poolid><ecid>rspipe_6_4</ecid></value>\n"
    attr.append(line)
    line="<value><poolid>clay_6_4_pool</poolid><ecid>clay_6_4</ecid></value>\n"
    attr.append(line)
    line="</attribute>\n"
    attr.append(line)
    
    line="</setting>\n"
    attr.append(line)
   
    filename="./sysSetting.xml_"+node
    print filename
    f=open(filename, "w")
    
    for line in attr:
        f.write(line)

    f.close()


# send conf to coordinator
coor="192.168.0.12"
filename="./sysSetting.xml_"+coor
cmd="scp "+filename+" "+coor+":/home/xiaolu/OpenEC/OpenEC-v3.0/conf/sysSetting.xml"
os.system(cmd)

for node in cluster:
    filename="./sysSetting.xml_"+node
    cmd="scp "+filename+" "+node+":/home/xiaolu/OpenEC/OpenEC-v3.0/conf/sysSetting.xml"
    print cmd
    os.system(cmd)

