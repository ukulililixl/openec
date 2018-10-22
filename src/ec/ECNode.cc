#include "ECNode.hh"

ECNode::ECNode(int id) {
  _nodeId = id;
  _hasConstraint = false;
  _consId = -1;
}

ECNode::~ECNode() {
}

void ECNode::addCoefs(int calfor, vector<int> coefs) {
  if (_coefMap.find(calfor) != _coefMap.end()) {
    _coefMap[calfor].clear();
    _coefMap[calfor] = coefs;
  } else {
    _coefMap.insert(make_pair(calfor, coefs));
  }
}

void ECNode::cleanChilds() {
  _childNodes.clear();
  _coefMap.clear();
}

void ECNode::setChilds(vector<ECNode*> childs) {
  _childNodes = childs; 
}

int ECNode::getChildNum() {
  return _childNodes.size();
}

vector<ECNode*> ECNode::getChildren() {
  return _childNodes;
}

void ECNode::incRefNumFor(int id) {
  if (_refNumFor.find(id) == _refNumFor.end()) _refNumFor.insert(make_pair(id, 1));
  else _refNumFor[id]++;
}

int ECNode::getRefNumFor(int id) {
  assert (_refNumFor.find(id) != _refNumFor.end());
  return _refNumFor[id];
}

void ECNode::dump(int parent) {
  if (parent == -1) parent = _nodeId;
  cout << "(data" << _nodeId;
  if (_childNodes.size() > 0) {
    cout << " = ";
  }
  vector<int> curCoef;
  if (_coefMap.size() > 1) {
    unordered_map<int, vector<int>>::const_iterator c = _coefMap.find(parent);
    assert (c!=_coefMap.end());
    curCoef = _coefMap[parent];
  } else if (_coefMap.size() == 1) {
    unordered_map<int, vector<int>>::const_iterator c = _coefMap.find(_nodeId);
    assert (c!=_coefMap.end());
    curCoef = _coefMap[_nodeId];
  }
  for (int i=0; i<_childNodes.size(); i++) {
    cout << curCoef[i] << " ";
    _childNodes[i]->dump(_nodeId);
    if (i < _childNodes.size() - 1) {
      cout << " + ";
    }
  }
  cout << ")";
}

int ECNode::getNodeId() {
  return _nodeId;
}
