#include "CmdDistributor.hh"

CmdDistributor::CmdDistributor(Config* conf) {
  _conf = conf;
  _dNum = _conf->_distThreadNum;
  
  _dThreads = vector<thread>(_dNum);
  for (int i=0; i<_dNum; i++) {
    _dThreads[i] = thread([=]{distribute();});
  }
}

CmdDistributor::~CmdDistributor() {
  // should not reach here
  for (int i=0; i<_dNum; i++) {
    _dThreads[i].join();
  }
}

void CmdDistributor::distribute() {
  redisContext* selfCtx = RedisUtil::createContext(_conf->_coorIp);
  redisReply* rReply;
  while(true) {
    cout << "CmdDistributor::distribute.wait for request!" << endl;
    rReply = (redisReply*)redisCommand(selfCtx, "blpop dist_request 0");
    if (rReply -> type == REDIS_REPLY_NIL) {
      cerr << "CmdDistributor::distribute. empty queue!" << endl;
      freeReplyObject(rReply);
      continue;
    } else if (rReply -> type == REDIS_REPLY_ERROR) {
      cerr << "CmdDistributor::distribute. error!" << endl;
      freeReplyObject(rReply);
      continue;
    } else {
      unsigned int ip;
      memcpy((char*)&ip, rReply->element[1]->str, 4);
      ip = ntohl(ip);

      redisContext* sendCtx = RedisUtil::createContext(ip);
      redisReply* sendReply = (redisReply*)redisCommand(sendCtx, "RPUSH ag_request %b", 
                                                        rReply->element[1]->str+4, 
                                                        rReply->element[1]->len-4);
      freeReplyObject(sendReply);
      freeReplyObject(rReply);
      redisFree(sendCtx);
    }
  }
  redisFree(selfCtx);
} 
