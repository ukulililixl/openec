#include "common/Config.hh"
//#include "CoorCommand.hh"
//#include "OECInputStream.hh"
//#include "OECOutputStream.hh"

#include "inc/include.hh"
#include "util/RedisUtil.hh"
using namespace std;

void usage() {
  cout << "usage: ./OECClient write inputfile filename ecid online sizeinMB" << endl;
  cout << "       ./OECClient write inputfile filename ecpool offline sizeinMB" << endl;
  cout << "       ./OECClient read filename saveas" << endl;
  cout << "       ./OECClient startEncode" << endl;
  cout << "       ./OECClient startRepair" << endl;
}

// void read(string filename, string saveas) {
// 
//   string confpath("./conf/sysSetting.xml");
//   Config* conf = new Config(confpath);
// 
//   struct timeval time1, time2;
//   gettimeofday(&time1, NULL);
//   // 0. create OECInputStream and init
//   OECInputStream* instream = new OECInputStream(conf, filename);
//   instream->output2file(saveas);
//   instream->close();
// 
//   gettimeofday(&time2, NULL);
//   cout << "overall.read.duration: " << RedisUtil::duration(time1, time2)<< endl;
// 
//   delete instream;
//   delete conf;
// }
// 
// void write(string inputname, string filename, string ecidpool, string encodemode, int sizeinMB) {
//   string confpath("./conf/sysSetting.xml");
//   Config* conf = new Config(confpath);
//   struct timeval time1, time2, time3, time4;
//   gettimeofday(&time1, NULL);
// 
//   // 0. create input file
//   FILE* inputfile = fopen(inputname.c_str(), "rb");
// 
//   // 1. create OECOutputStream and init
//   OECOutputStream* outstream = new OECOutputStream(conf, filename, ecidpool, encodemode, sizeinMB);
//   gettimeofday(&time2, NULL);
// 
//   int sizeinBytes = sizeinMB * 1048576;
//   int num = sizeinBytes/conf->_pktSize;
//   cout << "num = " << num << endl;
//   srand((unsigned)time(0));
// 
//   for (int i=0; i<num; i++) {
//     char* buf = (char*)calloc(conf->_pktSize+4, sizeof(char));
//  
//     int tmplen = htonl(conf->_pktSize);
//     memcpy(buf, (char*)&tmplen, 4);
// 
//     fread(buf+4, conf->_pktSize, 1, inputfile);
//     outstream->write(buf, conf->_pktSize+4);
//     free(buf);
//   }  
// 
//   gettimeofday(&time3, NULL);
//   outstream->close();
//   gettimeofday(&time4, NULL);
// 
//   cout << "create OECOutputStream: " << RedisUtil::duration(time1, time2)<< endl;
//   cout << "write all data into redis: " << RedisUtil::duration(time2, time3) << endl;
//   cout << "wait for ack: " << RedisUtil::duration(time3, time4) << endl;
//   cout << "overall.write.duration: " << RedisUtil::duration(time1, time4) << endl;
//   struct timeval close1, close2;
//   gettimeofday(&close1, NULL);
//   fclose(inputfile);
//   gettimeofday(&close2, NULL);
//   cout << "overall.write.close inputfile: " << RedisUtil::duration(close1, close2) << endl;
//   delete outstream;
//   delete conf;
// }

int main(int argc, char** argv) {

  if (argc < 2) {
    usage();
    return -1;
  }

//  string reqType(argv[1]);
//  if (reqType == "write") {
//    cout << "argc = " << argc << endl;
//    if (argc != 7) {
//      usage();
//      return -1;
//    }
//    string inputfile(argv[2]);
//    string filename(argv[3]);
//    string ecid(argv[4]);
//    string mode(argv[5]);
//    int size = atoi(argv[6]);
//    if ((mode == "online") || (mode == "offline")) {
//      write(inputfile, filename, ecid, mode, size)    ;
//    } else {
//      cout << "Error encodemode: Only support online/offline encode mode" << endl;
//      return -1;
//    }   
//  } else if (reqType == "read") {
//    if (argc != 4) {
//      usage();
//      return -1;
//    }
//    string filename(argv[2]);
//    string saveas(argv[3]);
//    read(filename, saveas);
//  } else if (reqType == "startEncode") {
//    string confpath("./conf/sysSetting.xml");
//    Config* conf = new Config(confpath);    
//    // send coorCmd to coordinator?
//    CoorCommand* cmd = new CoorCommand();
//    cmd->buildType7(7);
//    cmd->sendTo(conf->_coorIp);
//    
//    delete cmd;
//  } else if (reqType == "startRepair") {
//    string confpath("./conf/sysSetting.xml");
//    Config* conf = new Config(confpath);
//    // send coorCmd to coordinator
//    CoorCommand* cmd = new CoorCommand();
//    cmd->buildType9(9);
//    cmd->sendTo(conf->_coorIp);
//  }

  return 0;
}
