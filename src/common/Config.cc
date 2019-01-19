#include "Config.hh"

Config::Config(std::string& filepath) {
   XMLDocument doc;
   doc.LoadFile(filepath.c_str());
   XMLElement* element;
   for(element = doc.FirstChildElement("setting")->FirstChildElement("attribute");
       element!=NULL;
       element=element->NextSiblingElement("attribute")){
     XMLElement* ele = element->FirstChildElement("name");
     std::string attName = ele -> GetText();
     if (attName == "controller.addr") {
       _coorIp = inet_addr(ele -> NextSiblingElement("value") -> GetText());
     } else if (attName == "agents.addr") {
       for (ele = ele -> NextSiblingElement("value"); ele != NULL; ele = ele -> NextSiblingElement("value")) {
         std::string networkloc = ele -> GetText();
         std::string tmpstring = networkloc.substr(1);
         size_t pos = tmpstring.find("/");
         std::string rack = tmpstring.substr(0, pos);
         std::string ipstr = tmpstring.substr(pos+1);
         unsigned int ip = inet_addr(ipstr.c_str());
         _agentsIPs.push_back(ip);
         _ip2Rack.insert(make_pair(ip, rack));
         std::unordered_map<string, std::vector<unsigned int>>::iterator it = _rack2Ips.find(rack);
         if (it != _rack2Ips.end()) {
           _rack2Ips[rack].push_back(ip);
         } else {
           std::vector<unsigned int> curRack;
           curRack.push_back(ip);
           _rack2Ips.insert(make_pair(rack, curRack));
         }
       }
    } else if (attName == "oec.agent.thread.num") {
      _agWorkerThreadNum = std::stoi(ele -> NextSiblingElement("value") -> GetText());
    } else if (attName == "oec.controller.thread.num") {
      _coorThreadNum = std::stoi(ele -> NextSiblingElement("value") -> GetText());
    } else if (attName == "oec.cmddist.thread.num") {
      _distThreadNum = std::stoi(ele -> NextSiblingElement("value") -> GetText());
    } else if (attName == "ec.concurrent.num") {
      _ec_concurrent = std::stoi(ele -> NextSiblingElement("value") -> GetText());
    } else if (attName == "local.addr") {
      _localIp = inet_addr(ele -> NextSiblingElement("value") -> GetText());
    } else if (attName == "packet.size") {
      _pktSize = std::stoi(ele -> NextSiblingElement("value") -> GetText());
    } else if (attName == "dss.type") {
      _fsType = ele->NextSiblingElement("value")->GetText();
//    } else if (attName == "control.policy") {
//      _control_policy = ele -> NextSiblingElement("value") -> GetText();
//    } else if (attName == "data.policy") {
//      _data_policy = ele -> NextSiblingElement("value") -> GetText();
//    } else if (attName == "encode.scheduling") {
//      _encode_scheduling = ele -> NextSiblingElement("value") -> GetText();
//    } else if (attName == "encode.policy") {
//      _encode_policy = ele -> NextSiblingElement("value") -> GetText();
//    } else if (attName == "repair.scheduling") {
//      _repair_scheduling = ele -> NextSiblingElement("value") -> GetText();
//    } else if (attName == "repair.policy") {
//      _repair_policy = ele -> NextSiblingElement("value") -> GetText();
//    } else if (attName == "repair.threshold") {
//      _repair_threshold = std::stoi(ele -> NextSiblingElement("value") -> GetText());
//    } else if (attName == "placetest.avoidlocal") {
//      std::string avoidlocal = ele->NextSiblingElement("value")->GetText();
//      if (avoidlocal == "true") _avoid_local = true;
//      else _avoid_local = false;
    } else if (attName == "dss.parameter") {
      std::string paramtext = ele->NextSiblingElement("value")->GetText();
      int start = 0;
      int end = 0;
      
      while ((end = paramtext.find(",", start)) != -1) {
        std::string curparam = paramtext.substr(start, end);
        _fsParam.push_back(curparam);
        start = end + 1;
      }
      _fsParam.push_back(paramtext.substr(start));
    } else if (attName == "fs.factory") {
      for (XMLElement* curval = ele->NextSiblingElement("value");
           curval!=NULL;
           curval = curval->NextSiblingElement("value")) {
        XMLElement* curele = curval;
        // fstype
        curele = curele->FirstChildElement("fstype");
        if (!curele) {
          cout << "wrong configuration for fs.factory!" << endl;
          exit(1);
        }
        std::string fstype = curele->GetText();
        // params
        std::vector<std::string> param;
        curele = curele->NextSiblingElement("param");
        if (!curele) {
          cout << "wrong configuration for fs.factory!";
          exit(1);
        }
        std::string paramtext = curele->GetText();
        int start = 0;
        int end = 0;
        
        while ((end = paramtext.find(",", start)) != -1) {
          std::string curparam = paramtext.substr(start, end);
          param.push_back(curparam);
          start = end + 1;
        }
        param.push_back(paramtext.substr(start));
        _fsFactory.insert(make_pair(fstype, param));
      }
     } else if (attName == "ec.policy") {
       for (XMLElement* curval = ele->NextSiblingElement("value");
            curval!=NULL;
            curval = curval->NextSiblingElement("value")) {
         XMLElement* curele = curval;
         // ecid
         curele = curele->FirstChildElement("ecid");
         if (!curele) {
           cout << "wrong configuration for ec.policy!" << endl;
           exit(1);
         }
         std::string id = curele->GetText();
         // class
         curele = curele->NextSiblingElement("class");
         if (!curele) {
           cout << "wrong configuration for ec.policy!" << endl;
           exit(1);
         }
         std::string classname = curele->GetText();
         // n
         curele = curele->NextSiblingElement("n");
         if (!curele) {
           cout << "wrong configuration for ec.policy!" << endl;
           exit(1);
         }
         int n = std::stoi(curele->GetText());
         // k
         curele = curele->NextSiblingElement("k");
         if (!curele) {
           cout << "wrong configuration for ec.policy!" << endl;
           exit(1);
         }
         int k = std::stoi(curele->GetText());
         // w
         curele = curele->NextSiblingElement("w");
         if (!curele) {
           cout << "wrong configuration for ec.policy!" << endl;
           exit(1);
         }
         int w = std::stoi(curele->GetText());
//         // locality
//         curele = curele->NextSiblingElement("locality") ;
//         if (!curele) {
//           cout << "wrong configuration for ec.policy!" << endl;
//           exit(1);
//         }
//         bool locality=false;
//         std::string localitystr = curele->GetText();
//         if (localitystr == "true") locality=true;
         // opt level
         int optlevel = -1; // by default we enable Bind as optimization
         if (curele->NextSiblingElement("opt")) {
           curele = curele->NextSiblingElement("opt");
           optlevel = std::stoi(curele->GetText());
         }
         // other params
         std::vector<std::string> param;
         curele = curele->NextSiblingElement("param");
         if (curele) {
           std::string paramtext = curele->GetText();
           int start = 0;
           int end = 0;
           
           while ((end = paramtext.find(",", start)) != -1) {
             std::string curparam = paramtext.substr(start, end);
             param.push_back(curparam);
             start = end + 1;
           }
           param.push_back(paramtext.substr(start));
         }
         ECPolicy* ecpolicy = new ECPolicy(id, classname, n, k, w, optlevel, param);
         _ecPolicyMap.insert(make_pair(id, ecpolicy));
       }
    } else if (attName == "offline.pool") {
      for (XMLElement* curval = ele->NextSiblingElement("value");
           curval!=NULL;
           curval = curval->NextSiblingElement("value")) {
        XMLElement* curele = curval;
        // poolid
        curele = curele->FirstChildElement("poolid");
        if (!curele) {
          cout << "wrong configuration for offline.pool!" << endl;
          exit(1);
        }
        std::string poolid = curele->GetText(); 
        // ecid
        curele = curele->NextSiblingElement("ecid");
        if (!curele) {
          cout << "wrong configuration for offline.pool!" << endl;
          exit(1);
        }
        std::string ecid = curele->GetText();
        // base obj size
        curele = curele->NextSiblingElement("base");
        int basesize;
        if (!curele) {
          basesize = 1;
        } else {
          basesize = std::stoi(curele -> GetText());
        }
        _offlineECMap.insert(make_pair(poolid, ecid));
        _offlineECBase.insert(make_pair(poolid, basesize));
      }
//      XMLElement* curele = ele -> NextSiblingElement("value") -> FirstChildElement("poolid");
//      std::string poolname = curele -> GetText();
//      curele = curele -> NextSiblingElement("ecid");
//      std::string ecid = curele -> GetText();
//      _offlineECMap.insert(make_pair(poolname, ecid));
     }
   }

   _fsFactory.insert(make_pair(_fsType, _fsParam));
}

Config::~Config() {
  for (auto it = _ecPolicyMap.begin(); it != _ecPolicyMap.end(); it++) {
    ECPolicy* ecpolicy = it->second;
    delete ecpolicy;
  }
}

