#ifndef _CONFIG_HH_
#define _CONFIG_HH_

#include "../ec/ECPolicy.hh"
#include "../inc/include.hh"
#include "../util/tinyxml2.h"

using namespace tinyxml2;

class Config {
  public:
    Config(std::string& filePath);
    ~Config();
 
    //ip
    unsigned int _localIp;
    std::vector<unsigned int> _agentsIPs;
    unsigned int _coorIp;
    std::unordered_map<unsigned int, std::string> _ip2Rack;
    std::unordered_map<string, std::vector<unsigned int>> _rack2Ips;

//    unsigned int _repairIp;
//
    //worker thread num
    int _agWorkerThreadNum;

    //coor thread num
    int _coorThreadNum;

    //cmddistributor thread num
    int _distThreadNum;

    // size
    int _pktSize;

    // fstype
    std::string _fsType;
    std::vector<std::string> _fsParam;
    std::unordered_map<string, std::vector<string>> _fsFactory;

//    std::string _fsIp;
//    int _fsPort;

    // placepolicy
    std::string _control_policy = "random";
    std::string _data_policy = "random";
    bool _avoid_local = false;

    // ecpolicymap
    std::unordered_map<std::string, ECPolicy*> _ecPolicyMap;

    // offline pool
    std::unordered_map<string, string> _offlineECMap;
    std::unordered_map<string, int> _offlineECBase;

    // ec scheduling
    std::string _encode_scheduling = "normal";  // normal, delay
    std::string _encode_policy = "random";      // random, balance
    std::string _repair_scheduling = "normal";  // normal, delay, threshold
    std::string _repair_policy = "random";      // random, balance
    int _repair_threshold = 10;
    int _ec_concurrent; // concurrent stripe num
};
#endif
