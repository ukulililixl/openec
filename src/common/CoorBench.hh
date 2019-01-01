#ifndef _COORBENCH_HH_
#define _COORBENCH_HH_

#include "Config.hh"
#include "../inc/include.hh"
#include "../protocol/CoorCommand.hh"

using namespace std;

class CoorBench {
  private:
    Config* _conf;
    int _id;
    int _number;

    redisContext* _localCtx;
    redisContext* _coorCtx;
    int _replyid;
  public:
    CoorBench(Config* conf, int id, int number);
    void close();
};

#endif
