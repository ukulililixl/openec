#ifndef _ECPOLICY_HH_
#define _ECPOLICY_HH_

#include "BUTTERFLY64.hh"
#include "DRC643.hh"
#include "DRC963.hh"
#include "ECBase.hh"
#include "IA.hh"
#include "RSBINDX.hh"
#include "RSCONV.hh"
#include "RSPIPE.hh"
#include "RSPPR.hh"
#include "WASLRC.hh"
#include "../inc/include.hh"

using namespace std;

class ECPolicy {
  private:
    string _id;
    string _classname;
    int _n;
    int _k;
    int _w;
    bool _locality = false;
    int _opt;

    vector<string> _param;
  public:
//    ECPolicy(string id, string classname, int n, int k, int w, bool locality, int opt, vector<string> param);
    ECPolicy(string id, string classname, int n, int k, int w, int opt, vector<string> param);
    ECBase* createECClass();
    string getPolicyId();
    int getN();
    int getK();
    int getW();
    bool getLocality();
    int getOpt();
};

#endif
