#ifndef _CMDDISTRIBUTOR_HH_
#define _CMDDISTRIBUTOR_HH_

#include "Config.hh"

#include "../inc/include.hh"
#include "../util/RedisUtil.hh"


using namespace std;

class CmdDistributor {
  private:
    Config* _conf;
    int _dNum;
    vector<thread> _dThreads;
    unordered_map<unsigned int, redisContext*> _agentCtx;

  public:
    CmdDistributor(Config* conf); 
    ~CmdDistributor();
    void distribute();
};

#endif

