#ifndef _REDISUTIL_HH_
#define _REDISUTIL_HH_


#include "../inc/include.hh"

#define REDIS_CREATION_FAILURE 1
#define REDIS_BLPOP_FAILURE 2
#define REDIS_RPUSH_FAILURE 3

using namespace std;

class RedisUtil {
  public:
    static string ip2Str(unsigned int ip);

    static redisContext* createContext(unsigned int ip);
    static redisContext* createContext(string ip);
    static redisContext* createContext(string ip, int port);

    // return length of the pop'ed content
    static int blpopContent(redisContext*, const char* key, char* dst, int length);
    static void rpushContent(redisContext*, const char* key, const char* src, int length);
    static double duration(struct timeval t1, struct timeval t2);
    static vector<string> str2container(string line);
};


#endif
