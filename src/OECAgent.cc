#include "common/Config.hh"
#include "common/OECWorker.hh"

#include "inc/include.hh"

using namespace std;

int main(int argc, char** argv) {

  string configPath = "conf/sysSetting.xml";
  Config* conf = new Config(configPath);

  OECWorker** workers = (OECWorker**)calloc(conf -> _agWorkerThreadNum, sizeof(OECWorker*)); 

  thread thrds[conf -> _agWorkerThreadNum];
  for (int i = 0; i < conf -> _agWorkerThreadNum; i ++) {
    workers[i] = new OECWorker(conf);
    thrds[i] = thread([=]{workers[i] -> doProcess();});
  }
  cout << "OECAgent started ..." << endl;

  /**
   * Shoule never reach here
   */
  for (int i = 0; i < conf -> _agWorkerThreadNum; i ++) {
    thrds[i].join();
  }
  for (int i=0; i<conf -> _agWorkerThreadNum; i++) {
    delete workers[i];
  }
  free(workers);
  delete conf;

  return 0;
}

