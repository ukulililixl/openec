#include "Cluster.hh"

Cluster::Cluster(vector<int> childs, int parent) {
  _childs = childs;
  sort(_childs.begin(), _childs.end());
  _parents.push_back(parent);

  _opted = -1; // this cluster has never been optimized;
}

bool Cluster::childsInCluster(vector<int> childs) {
  if (CLUSTER_DEBUG_ENABLE) {
    cout << "Cluster::childsInCluster.my childs: ( ";
    for (auto c: _childs) cout << c << " ";
    cout << "), input childs: ( ";
    for (auto c: childs) cout << c << " ";
    cout << ") " << endl;
  }
  if (childs.size() != _childs.size()) return false;

  for (auto child: childs) {
    if (find(_childs.begin(), _childs.end(), child) == _childs.end()) return false;
  }
  return true;
}

void Cluster::addParent(int parent) {
  _parents.push_back(parent);
}

void Cluster::setOpt(int opt) {
  _opted = opt;
}

void Cluster::dump() {
  cout << "Cluster:: ( ";
  for (int i=0; i<_childs.size(); i++) cout << _childs[i] << " ";
  cout << ") -> ( ";
  for (int i=0; i<_parents.size(); i++) cout << _parents[i] << " ";
  cout << ") , opted: " << _opted << endl;
}

int Cluster::getOpt() {
  return _opted;
}

vector<int> Cluster::getParents() {
  return _parents;
}

vector<int> Cluster::getChilds() {
  return _childs;
}
