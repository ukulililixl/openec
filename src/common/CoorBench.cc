#include "CoorBench.hh"

CoorBench::CoorBench(Config* conf, int benchid, int number) {
  _conf = conf;
  _id = benchid;
  _number = number;

  redisContext* localCtx = RedisUtil::createContext(_conf->_localIp);
  redisContext* _coorCtx = RedisUtil::createContext(_conf->_coorIp);
  redisReply* rReply;
  _replyid=0;

  // start client one by one?
  for (int i=0; i<number; i++) {
    string benchname = "bench:"+to_string(benchid)+":"+to_string(i);
    CoorCommand* coorCmd = new CoorCommand();
    coorCmd->buildType12(12, _conf->_localIp, benchname);
    coorCmd->sendTo(_coorCtx);

    if (i >= 0) {
      string benchname = "bench:"+to_string(_id)+":"+to_string(_replyid++);
      string key="benchfinish:"+benchname;
      rReply = (redisReply*)redisCommand(localCtx, "blpop %s 0", key.c_str());
      freeReplyObject(rReply);
    }

    delete coorCmd;
  }
  redisFree(localCtx);
}

void CoorBench::close() {
  redisReply* rReply;
  redisContext* localCtx = RedisUtil::createContext(_conf->_localIp);
  for (int i=_replyid; i<_number; i++) {
    string benchname = "bench:"+to_string(_id)+":"+to_string(i);
    string key="benchfinish:"+benchname;
    rReply = (redisReply*)redisCommand(localCtx, "blpop %s 0", key.c_str());
    freeReplyObject(rReply);
  }
  redisFree(localCtx);
  redisFree(_coorCtx);
}
