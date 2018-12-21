#include "CoorBench.hh"

CoorBench::CoorBench(Config* conf, int benchid, int number) {
  _conf = conf;
  _id = benchid;
  _number = number;

  // start client one by one?
  for (int i=0; i<number; i++) {
    string benchname = "bench:"+to_string(benchid)+":"+to_string(i);
    CoorCommand* coorCmd = new CoorCommand();
    coorCmd->buildType12(12, _conf->_localIp, benchname);
    coorCmd->sendTo(_conf->_coorIp);

    delete coorCmd;
  }
}

void CoorBench::close() {
  redisContext* localCtx = RedisUtil::createContext(_conf->_localIp);
  redisReply* rReply;
  for (int i=0; i<_number; i++) {
    string benchname = "bench:"+to_string(_id)+":"+to_string(i);
    string key="benchfinish:"+benchname;
    rReply = (redisReply*)redisCommand(localCtx, "blpop %s 0", key.c_str());
    freeReplyObject(rReply);
  }
  redisFree(localCtx);
}
