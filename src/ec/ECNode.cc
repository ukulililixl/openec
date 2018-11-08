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

int ECNode::getNodeId() {
  return _nodeId;
}

unordered_map<int, vector<int>> ECNode::getCoefmap() {
  return _coefMap;
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

void ECNode::parseForClient(vector<ECTask*>& tasks) {
  // we parse compute task for client
  bool computebool = false;
  // if the number of children is larger than 1, there must be calculation
  if (_childNodes.size() > 1) computebool = true;
  else if (_childNodes.size() == 0) computebool = false;
  else {
    // if the number of children equals to 1, 
    ECNode* childNode = _childNodes[0];
    // 1> this node is linked to a bind node, there is no calculation
    if (childNode->getCoefmap().size() > 1) computebool = false;
    // 2> this node is not linked to a bind node, there is calculation of res = coef * value
    else if (childNode->getCoefmap().size() == 1) computebool = true;
  }

  if (computebool) {
    ECTask* compute = new ECTask();
    compute->setType(2);
    vector<int> children;
    for (int i=0; i<_childNodes.size(); i++) children.push_back(_childNodes[i]->getNodeId());
    compute->setChildren(children);
    compute->setCoefmap(_coefMap);
    tasks.push_back(compute);
  }
}

// unordered_map<int, ECTask*> ECNode::createTasks() {
//   unordered_map<int, ECTask*> toret;
// 
//   // 0. check for Load task
//   // if the node does not have any children, then it is leaf and requires Load Task 
//   bool loadbool = false;
//   if (_childNodes.size() == 0) loadbool = true;
//   if (loadbool) {
//     // there is Load task
//     ECTask* load = new ECTask();
//     load->setType(0);
//     load->setIdx(_nodeId); // cidx; when w == 1, this sidx = cidx; when w > 1, 
//     toret.insert(make_pair(0, load));
//   }
//   // 1. check for fetch task
//   // if the node has at least one child, then it requires Fetch Task
//   bool fetchbool = false;
//   // 1.1 if there are multiple children, then it requires Fetch Task
//   if (_childNodes.size() > 1) fetchbool = true;
//   // 1.2 if there is only 1 children, we check whether this child is only linked to a bindnode (the node created from BindX)
//   if (_childNodes.size() == 1) {
//     ECNode* curchildnode = _childNodes[0];
//     vector<ECNode*> grandchildren = curchildnode->getChildren();
//     // 1.2.1 if number of grandchildren > 1, then child is not only linked to a bindnode(node that is created from BindX)
//     if (grandchildren.size() > 1) fetchbool = true;
//     // 1.2.2 if granchildren == 1, then check whether grandchild is a bindnode
//     if (grandchildren.size() == 1) {
//       ECNode* grandchild = grandchildren[0];
//       if (grandchild->getCoefmap().size() > 1 && grandchild->getChildren().size() > 1) {
//         // grandchild is a bind node, there is no fetch task
//         // however, we need to record where is result is when we create task for the bindnode
//         fetchbool = false;
//       }
//     }
//   }
//   if (fetchbool) {
//     // there is fetch task
//     ECTask* fetch = new ECTask();
//     fetch->setType(1);
//     vector<int> children;
//     for (int i=0; i<_childNodes.size(); i++) children.push_back(_childNodes[i]->getNodeId());
//     fetch->setChildren(children);
//     toret.insert(make_pair(1, fetch));
//   }
// 
//   // 2. check for compute task
//   bool computebool = false;
//   if (_childNodes.size() > 1) computebool = true;
//   if (_childNodes.size() == 1 && _coefMap.size() == 1) {
//     // check whether coef is 1
//     for (auto item: _coefMap) {
//       if (item.second.size() == 1 && item.second[0] != 1) computebool = true;
//       break;
//     }
//   }
//   if (computebool) {
//     ECTask* compute = new ECTask();
//     compute->setType(2);
//     vector<int> children;
//     for (int i=0; i<_childNodes.size(); i++) children.push_back(_childNodes[i]->getNodeId());
//     compute->setChildren(children);
//     compute->setCoefmap(_coefMap);
//     toret.insert(make_pair(2, compute));
//   }
// 
// //  // 3. check for persist task
// //  // actually, persist task is needed for every operation
// //  // the difference is that some persist to DSS while some persist to redis
// //  ECTask* persist = new ECTask();
// //  persist->setType(3);
// //  persist->setRefnum(_refNumFor);
// //  toret.insert(make_pair(3, persist));
// 
// //  // 4. review tasks?
// //  for (auto item: toret) {
// //    item.second->dump();
// //  }
//   return toret;
// }
