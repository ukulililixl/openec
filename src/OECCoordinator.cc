#include "common/CmdDistributor.hh"
#include "common/Config.hh"
#include "common/Coordinator.hh"
#include "common/StripeStore.hh"

#include "inc/include.hh"


using namespace std;

int main(int argc, char** argv) {
  
  string configpath = "conf/sysSetting.xml";
  Config* conf = new Config(configpath);
  // create stripestore
  // TODO: need to add recover from backup
  StripeStore* ss = new StripeStore(conf); 
  thread scanThread = thread([=]{ss->scanning();});
  thread rpThread = thread([=]{ss->scanRepair();});

  // command distributor
  CmdDistributor* cmdDistributor = new CmdDistributor(conf);

  // coordinator
  Coordinator** coors = (Coordinator**)calloc(conf->_coorThreadNum, sizeof(Coordinator*));
  thread thrds[conf->_coorThreadNum];
  for (int i=0; i<conf->_coorThreadNum; i++) {
    coors[i] = new Coordinator(conf, ss);
    thrds[i] = thread([=]{coors[i]->doProcess();});
  }
  cout << "OECCoordinator started ......" << endl;
  /**
   * Shoule never reach here
   */
  for (int i=0; i<conf->_coorThreadNum; i++) {
    thrds[i].join();
  }
  for (int i=0; i<conf->_coorThreadNum; i++) {
    delete coors[i];
  }
  rpThread.join();
  scanThread.join();
  free(coors);
  delete conf;
  delete cmdDistributor;

  return 0;
}
