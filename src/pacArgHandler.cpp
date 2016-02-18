#include "pacStable.h"
#include "pacArgHandler.h"
#include "pacIntrinsicArgHandler.h"
#include "pacStringUtil.h"
#include "pacStdUtil.h"
#include "pacConsole.h"

namespace pac {

//------------------------------------------------------------------
void Branch::recordNodeValue(Node* node, SVCIter f, SVCIter l) {
  PacAssert(node, "0 node");
  if (node->isLeaf() || node->isRoot())
    PAC_EXCEPT(Exception::ERR_INVALID_STATE,
        "you can not add node value to root or leaf.");

  NodeValues::iterator iter =
      std::find_if(this->nodeValues.begin(), this->nodeValues.end(),
          [&](NodeValues::value_type& v) -> bool { return v.first == node; });
  sgLogger.logMessage(
      "record node value " + node->getName() + " : " + StringUtil::join(f, l),
      SL_TRIVIAL);

  if (iter == this->nodeValues.end()) {
    this->nodeValues.push_back(
        std::make_pair(node, std::vector<SVCIterPair>()));
    this->nodeValues.rbegin()->second.push_back(std::make_pair(f, l));
  } else {
    if (!node->getLoopNode())
      PAC_EXCEPT(Exception::ERR_INVALID_STATE,
          "you can not record node value twice for the same normal node which "
          "has no loop type ancestor node");
    iter->second.push_back(std::make_pair(f, l));
  }
}

//------------------------------------------------------------------
void Branch::recordTreeLeafPair(TreeArgHandler* tree, Node* leaf) {
  sgLogger.logMessage("recored subtree " + tree->getName() +
                          " with matched branch" + leaf->getBranch(),
      SL_TRIVIAL);
  treeLeafPairs.push_back(std::make_pair(tree, leaf));
}

//------------------------------------------------------------------
void Branch::pushTree(TreeArgHandler* tree) {
  treeStartPairs.push(std::make_pair(tree, current));
}

//------------------------------------------------------------------
TreeStartPair& Branch::topTree() { return treeStartPairs.top(); }

//------------------------------------------------------------------
void Branch::popTree(Node* leaf) {
  TreeStartPair& tlp = topTree();
  TreeArgHandler* tree = tlp.first;
  if (tree != leaf->getTree())
    PAC_EXCEPT(Exception::ERR_INVALID_STATE,
        "tree of leaf(" + leaf->getTree()->getName() +
            ") is different from top tree:" + tree->getName());

  Node* treeNode = tree->getTreeNode();
  if (treeNode) {
    // record it's value and matched leaf if it's a sub tree
    this->recordNodeValue(treeNode, tlp.second, this->current);
    this->recordTreeLeafPair(tree, leaf);
  } else {
    // main tree, main tree has no parent node, and it's value is unique
    tlp.first->setValue(StringUtil::join(tlp.second, this->current));
    tlp.first->setMatchedLeaf(leaf);
  }

  treeStartPairs.pop();
}

//------------------------------------------------------------------
void Branch::restoreBranch() {
  Node* node = getLastNode();
  PacAssert(node && node->getLeafChild(),
      "illegal state, found no leaf child, invalid branch");
  PacAssert(!node->getTree()->getTreeNode(),
      "illegal state,  node doesn't belong to main tree");
  PacAssert(treeStartPairs.size() == 0, "there should no trees left in stack");

  // restore node values
  std::for_each(nodeValues.begin(), nodeValues.end(),
      [&](NodeValues::value_type& v) -> void {
        std::for_each(v.second.begin(), v.second.end(),
            [&](SVCIterPair& iterPair) -> void {
              v.first->restoreValue(iterPair.first, iterPair.second);
            });
      });

  // restore tree leaf
  std::for_each(treeLeafPairs.begin(), treeLeafPairs.end(),
      [&](TreeLeafPair& v) -> void { v.first->setMatchedLeaf(v.second); });
}

//------------------------------------------------------------------
Node* Branch::getLastNode() {
  PacAssert(!nodeValues.empty(), "found no recorded node values");
  return std::get<0>(nodeValues[nodeValues.size() - 1]);
}

//------------------------------------------------------------------
ArgHandler::ArgHandler(const std::string& name)
    : mArgHandlerType(AHT_PRIMITIVE),
      mPromptType(PT_PROMPTONLY),
      mNode(0),
      mName(name) {}

//------------------------------------------------------------------
void ArgHandler::prompt(const std::string& s) {
  this->runtimeInit();
  this->populatePromptBuffer(s);
  this->applyPromptBuffer(s);
}

//------------------------------------------------------------------
bool ArgHandler::validate(const std::string& s) {
  this->runtimeInit();
  bool res = this->doValidate(s);
  if (res) this->setValue(s);
  return res;
}

//------------------------------------------------------------------
void ArgHandler::validateBranch(BranchVec& branches) {
  PacAssert(!branches.empty(), "empty branch");
  BranchVec::iterator iter =
      std::remove_if(branches.begin(), branches.end(), [&](Branch& v) -> bool {
        if (v.current == v.last || !validate(*v.current))
          return true;
        else {
          v.recordNodeValue(this->getTreeNode(), v.current, v.current + 1);
          ++v.current;
          return false;
        }
      });

  branches.erase(iter, branches.end());
}

//------------------------------------------------------------------
void ArgHandler::applyPromptBuffer(const std::string& s, bool autoComplete) {
  if (mPromptBuffer.empty()) return;

  if (mPromptType == PT_PROMPTONLY) {
    std::for_each(mPromptBuffer.begin(), mPromptBuffer.end(),
        [&](const std::string& v) -> void { sgConsole.outputLine(v); });
  } else {
    if (mPromptBuffer.size() > 1 || !autoComplete) {
      RaiiConsoleBuffer();
      std::for_each(mPromptBuffer.begin(), mPromptBuffer.end(),
          [&](const std::string& v) -> void { sgConsole.output(v); });
    }
    if (autoComplete) this->completeTyping(s);
  }
}

//------------------------------------------------------------------
void ArgHandler::outputErrMessage(const std::string& s) {
  sgConsole.outputLine(s + " is not a valid " + getName());
}

//------------------------------------------------------------------
StringVector::iterator ArgHandler::beginPromptBuffer() {
  return mPromptBuffer.begin();
}

//------------------------------------------------------------------
StringVector::iterator ArgHandler::endPromptBuffer() {
  return mPromptBuffer.end();
}

//------------------------------------------------------------------
size_t ArgHandler::getPromptBufferSize() { return mPromptBuffer.size(); }

//------------------------------------------------------------------
void ArgHandler::getPromptArgHandlers(ArgHandlerVec& ahv) {
  ahv.push_back(this);
}

//------------------------------------------------------------------
void ArgHandler::appendPromptBuffer(const std::string& buf) {
  mPromptBuffer.push_back(buf);
}

//------------------------------------------------------------------
void ArgHandler::completeTyping(const std::string& s) {
  RaiiConsoleBuffer();
  const std::string&& iden =
      StdUtil::getIdenticalString(mPromptBuffer.begin(), mPromptBuffer.end());
  // just check again
  if (!StringUtil::startsWith(iden, s))
    PAC_EXCEPT(Exception::ERR_INVALID_STATE,
        "complete string done's starts with typing!!!!");

  sgConsole.complete(iden.substr(s.size()));
}

//------------------------------------------------------------------
Node::Node(const std::string& name, const std::string& ahName, NodeType nt)
    : mNodeType(nt),
      mParent(0),
      mArgHandler(0),
      mTree(0),
      mName(name),
      mAhName(ahName) {
  buildArgHandler();
  // if (isLoop() && mArgHandler->getArgHandlerType() == ArgHandler::AHT_TREE)
  // PAC_EXCEPT(Exception::ERR_INVALID_STATE, "you can not loop tree");
}

//------------------------------------------------------------------
Node::~Node() {
  std::for_each(
      mChildren.begin(), mChildren.end(), [&](Node* v) -> void { delete v; });
  mChildren.clear();
}

//------------------------------------------------------------------
Node::Node(const Node& rhs)
    : mNodeType(rhs.mNodeType),
      mParent(0),
      mArgHandler(0),
      mTree(0),
      mBranch(rhs.mBranch),
      mName(rhs.getName()),
      mAhName(rhs.getAhName()) {
  // copy handler
  buildArgHandler();

  // deep copy children, take care of loop type.
  std::for_each(rhs.beginChildIter(), rhs.endChildIter(),
      [&](Node* v) -> void { this->addChildNode(new Node(*v)); });
}

//------------------------------------------------------------------
size_t Node::getDepth() {
  size_t d = 0;
  Node* node = this;
  while ((node = node->getParent())) ++d;

  return d;
}

//------------------------------------------------------------------
Node* Node::addChildNode(Node* child) {
  mChildren.push_back(child);
  if (child != this)  // loop type can add it self to child
  {
    child->setParent(this);
    child->setTree(mTree);
  }
  return child;
}

//------------------------------------------------------------------
Node* Node::addChildNode(const std::string& name, const std::string& ahName,
    NodeType nt /*= NT_NORMAL*/) {
  Node* node = new Node(name, ahName, nt);
  return this->addChildNode(node);
}

//------------------------------------------------------------------
Node* Node::getChildNode(const std::string& name, bool recursive /*= 1*/) {
  for (NodeVector::iterator iter = mChildren.begin(); iter != mChildren.end();
       ++iter) {
    if (isLeaf()) continue;

    if ((*iter)->getName() == name) return *iter;

    if (recursive) {
      Node* node = (*iter)->getChildNode(name, 1);
      if (node) {
        int j = 5;
        ++j;
        return node;
      }
    }
  }

  return 0;
}

//------------------------------------------------------------------
Node* Node::getAncestorNode(const std::string& name) {
  if (!mParent) return 0;

  if (mParent->getName() == name)
    return mParent;
  else
    return mParent->getAncestorNode(name);
}

//------------------------------------------------------------------
Node* Node::endBranch(const std::string& branchName) {
  Node* tail = new Node(
      this->getTree()->getName() + "_tail_" + branchName, "", Node::NT_LEAF);
  return this->addChildNode(tail);
}

//------------------------------------------------------------------
void Node::validateBranch(BranchVec& branches) {
  PacAssert(!branches.empty(), "empty branch");
  sgLogger.logMessage("node " + this->mName + " validateBranch with " +
                          StringUtil::toString(branches.size()) + "branches",
      SL_TRIVIAL);

  if (isLeaf()) {
    // recored tree values
    sgLogger.logMessage("meet leaf of " + this->getTree()->getName() +
                            " with " + StringUtil::toString(branches.size()) +
                            " branches",
        SL_TRIVIAL);
    std::for_each(branches.begin(), branches.end(),
        [&](Branch& branch) -> void { branch.popTree(this); });
    return;
  }

  if (isRoot()) {
    // record tree start point
    std::for_each(branches.begin(), branches.end(),
        [&](Branch& v) -> void { v.pushTree(getTree()); });
  } else {
    // test current arghandler
    PacAssert(mArgHandler, "0 arghandler");
    this->mArgHandler->validateBranch(branches);
    if (branches.empty()) return;
  }

  BranchVec origBranch(branches);
  branches.clear();

  // loop node
  if (isLoop()) {
    BranchVec tmp(origBranch);
    this->validateBranch(tmp);
    branches.insert(branches.end(), tmp.begin(), tmp.end());
  }

  // add branches of child node
  std::for_each(mChildren.begin(), mChildren.end(), [&](Node* node) -> void {
    BranchVec tmp(origBranch);
    node->validateBranch(tmp);
    branches.insert(branches.end(), tmp.begin(), tmp.end());
  });
}

//------------------------------------------------------------------
void Node::restoreValue(SVCIter first, SVCIter last) {
  if (last == first)
    PAC_EXCEPT(Exception::ERR_INVALID_STATE, "invalid 0 values for loop node");

  if (isLeaf() || isRoot())
    PAC_EXCEPT(
        Exception::ERR_INVALID_STATE, "you can not set value for root or leaf");

  const std::string&& value = StringUtil::join(first, last);
  if (mArgHandler->getArgHandlerType() == ArgHandler::AHT_TREE) {
    mArgHandler->setValue(value);
  } else {
    // for primitive type, set it's value to last valid value
    mArgHandler->setValue(*(last - 1));
  }

  if (this->getLoopNode())
    // recored value under loop type node
    mValues.push_back(value);
}

//------------------------------------------------------------------
NodeVector Node::getLeaves() {
  NodeVector nv;
  if (isLeaf()) {
    nv.push_back(this);
    return nv;
  }

  std::for_each(mChildren.begin(), mChildren.end(), [&](Node* v) -> void {
    NodeVector&& childLeaves = v->getLeaves();
    nv.insert(nv.end(), childLeaves.begin(), childLeaves.end());
  });
  return nv;
}

//------------------------------------------------------------------
std::string Node::getArgPath() {
  PacAssertS(isLeaf(), "You can not call getArgPath with type :" +
                           StringUtil::toString(static_cast<int>(mNodeType)));

  StringVector sv;
  Node* node;
  while ((node = getParent()) && !node->isRoot()) {
    sv.push_back(node->getValue());
  }

  return StringUtil::join(sv);
}

//------------------------------------------------------------------
ArgHandler* Node::getArgHandler() const {
  PacAssert(mArgHandler, "0 arghandler");
  return mArgHandler;
}

//------------------------------------------------------------------
void Node::buildArgHandler() {
  if (!isRoot() && !isLeaf()) {
    if (mAhName.empty())
      PAC_EXCEPT(Exception::ERR_INVALID_STATE, "empty ahName");
    mArgHandler = sgArgLib.createArgHandler(mAhName);
    PacAssert(mArgHandler, "0 arg handler");
    mArgHandler->setTreeNode(this);
  } else {
    if (!mAhName.empty())
      PAC_EXCEPT(Exception::ERR_INVALID_STATE, "not empty ahName");
  }
}

//------------------------------------------------------------------
Node* Node::getLeafChild() const {
  NodeVector::const_iterator iter =
      std::find_if(mChildren.begin(), mChildren.end(),
          [&](const NodeVector::value_type& v) -> bool { return v->isLeaf(); });

  return iter != mChildren.end() ? *iter : 0;
}

//------------------------------------------------------------------
Node* Node::getLoopNode() {
  if (isLoop()) return this;
  Node* treeNode = this->getTree()->getTreeNode();
  if (!treeNode) return 0;
  return treeNode->getLoopNode();
}

//------------------------------------------------------------------
void Node::setTree(TreeArgHandler* tree, bool recursive /*= true*/) {
  mTree = tree;
  if (recursive) {
    std::for_each(mChildren.begin(), mChildren.end(),
        [&](Node* v) -> void { v->setTree(tree, recursive); });
  }
}

//------------------------------------------------------------------
const std::string& Node::getBranch() {
  PacAssertS(isLeaf(), "You can not call getBranch with type :" +
                           StringUtil::toString(static_cast<int>(mNodeType)));
  return mBranch;
}

//------------------------------------------------------------------
void Node::setBranch(int v) {
  PacAssertS(isLeaf(), "You can not call getBranch with type :" +
                           StringUtil::toString(static_cast<int>(mNodeType)));
  mBranch = v;
}

//------------------------------------------------------------------
NodeVector::const_iterator Node::beginChildIter() const {
  return mChildren.begin();
}

//------------------------------------------------------------------
NodeVector::const_iterator Node::endChildIter() const {
  return mChildren.end();
}

//------------------------------------------------------------------
size_t Node::getNumChildren() const { return mChildren.size(); }

//------------------------------------------------------------------
StringVector::const_iterator Node::beginValueIter() const {
  return mValues.begin();
}

//------------------------------------------------------------------
StringVector::const_iterator Node::endValueIter() const {
  return mValues.end();
}

//------------------------------------------------------------------
void Node::getPromptArgHandlers(ArgHandlerVec& ahv) {
  if (isLeaf()) return;

  std::for_each(mChildren.begin(), mChildren.end(), [&](Node* v) -> void {
    if (!v->isLeaf()) v->getArgHandler()->getPromptArgHandlers(ahv);
  });
  if (isLoop()) mArgHandler->getPromptArgHandlers(ahv);
}

//------------------------------------------------------------------
TreeArgHandler::TreeArgHandler(const std::string& name)
    : ArgHandler(name), mMatchedLeaf(0) {
  mRoot = new Node(name + "_root", "", Node::NT_ROOT);
  mRoot->setTree(this);
  setArgHandlerType(AHT_TREE);
}

//------------------------------------------------------------------
TreeArgHandler::TreeArgHandler(const TreeArgHandler& rhs)
    : ArgHandler(rhs), mMatchedLeaf(0) {
  mRoot = new Node(*rhs.getRoot());
  mRoot->setTree(this);
  // make sure no duplicate branch exists
  NodeVector&& leaves = mRoot->getLeaves();
  NodeVector::iterator iter = std::unique(leaves.begin(), leaves.end());
  PacAssert(iter == leaves.end(), "duplicate tree branches");
}

//------------------------------------------------------------------
void TreeArgHandler::prompt(const std::string& s) {
  this->runtimeInit();
  // split by space
  StringVector sv;
  if (!s.empty()) sv = StringUtil::split(s);

  if (s.empty() || s[s.size() - 1] == ' ') sv.push_back("");

  BranchVec branches;
  branches.push_back(Branch(sv.begin(), sv.end() - 1, sv.begin()));
  this->validateBranch(branches);

  ArgHandlerVec ahv;
  // filter result, get candidate nodes;
  std::for_each(
      branches.begin(), branches.end(), [&](BranchVec::value_type& v) -> void {
        if (v.current != v.last) return;
        Node* n = std::get<0>(v.nodeValues[v.nodeValues.size() - 1]);
        n->getPromptArgHandlers(ahv);
      });

  if (ahv.size() == 1)  // one candidate
    ahv[0]->prompt(s);
  else if (ahv.size() > 1) {
    // mutiple candidates
    std::for_each(ahv.begin(), ahv.end(), [&](ArgHandler* handler) -> void {
      handler->runtimeInit();
      handler->populatePromptBuffer(sv[sv.size() - 1]);
      if (handler->getPromptBufferSize() > 0) {
        // output head
        sgConsole.outputLine(handler->getTreeNode()->getArgPath() + ":");
        handler->applyPromptBuffer(s, false);
      }
    });
  }
}

//------------------------------------------------------------------
bool TreeArgHandler::validate(const std::string& s) {
  sgLogger.logMessage("Tree " + mName + " start validate " + s, SL_TRIVIAL);
  this->checkWholeness();

  this->runtimeInit();
  // split by space
  StringVector sv;
  if (!s.empty()) sv = StringUtil::split(s);

  BranchVec branches;
  branches.push_back(Branch(sv.begin(), sv.end(), sv.begin()));
  this->validateBranch(branches);

  // filter result, arg must be totally consumed and each node must has a leaf
  // child
  BranchVec::iterator iter = std::remove_if(
      branches.begin(), branches.end(), [&](BranchVec::value_type& v) -> bool {
        if (v.current != v.last) return true;
        Node* n = v.getLastNode();
        while (n) {
          if (!n->getLeafChild()) return true;
          n = n->getTree()->getTreeNode();
        }
        return false;
      });
  branches.erase(iter, branches.end());

  sgLogger.logMessage(
      "found " + StringUtil::toString(branches.size()) + " branches",
      SL_TRIVIAL);

  // make sure only 1 matched branch exists
  if (branches.size() > 1) {
    sgConsole.outputLine("found multiple valid branches:");
    std::for_each(branches.begin(), branches.end(), [&](Branch& arg) -> void {
      sgConsole.outputLine(arg.getLastNode()->getLeafChild()->getArgPath());
    });
    return false;
  } else if (branches.size() == 0) {
    return false;
  } else {
    branches[0].restoreBranch();
    return true;
  }
}

//------------------------------------------------------------------
void TreeArgHandler::validateBranch(BranchVec& branches) {
  this->checkWholeness();
  this->runtimeInit();
  getRoot()->validateBranch(branches);
}

//------------------------------------------------------------------
void TreeArgHandler::outputErrMessage(const std::string& s) {
  sgConsole.outputLine(
      "Illigal format,  " + getName() + " takes following formats:");
  NodeVector&& nv = getLeaves();
  std::for_each(nv.begin(), nv.end(),
      [&](Node* v) -> void { sgConsole.outputLine(v->getArgPath()); });
}

//------------------------------------------------------------------
Node* TreeArgHandler::getNode(const std::string& name) {
  Node* node = mRoot->getChildNode(name);
  if (node == 0)
    PAC_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
        name + " not found in tree handler" + getName());
  return node;
}

//------------------------------------------------------------------
const std::string& TreeArgHandler::getNodeValue(const std::string& name) {
  return getNode(name)->getValue();
}

//------------------------------------------------------------------
NodeVector TreeArgHandler::getLeaves() { return mRoot->getLeaves(); }

//------------------------------------------------------------------
const std::string& TreeArgHandler::getMatchedBranch() {
  PacAssertS(mMatchedLeaf, "Havn't found matched leaf for " + getName());
  return mMatchedLeaf->getBranch();
}

//------------------------------------------------------------------
Node* TreeArgHandler::getMatchedNode(const std::string& name) {
  return getMatchedLeaf()->getAncestorNode(name);
}

void checkWholeness(Node* n) {
  if (n->isLeaf()) return;

  if (n->getNumChildren() == 0)
    PAC_EXCEPT(Exception::ERR_INVALID_STATE, "do you  forget to endbranch for" +
                                                 n->getTree()->getName() +
                                                 " at " + n->getName());

  std::for_each(n->beginChildIter(), n->endChildIter(),
      [&](Node* v) -> void { checkWholeness(v); });
}

//------------------------------------------------------------------
void TreeArgHandler::checkWholeness() { pac::checkWholeness(getRoot()); }

//------------------------------------------------------------------
void TreeArgHandler::getPromptArgHandlers(ArgHandlerVec& ahv) {
  return getRoot()->getPromptArgHandlers(ahv);
}

//------------------------------------------------------------------
ArgHandlerLib::~ArgHandlerLib() {
  std::for_each(mArgHandlerMap.begin(), mArgHandlerMap.end(),
      [&](ArgHandlerMap::value_type& v) -> void { delete v.second; });
  mArgHandlerMap.clear();
}

//------------------------------------------------------------------
void ArgHandlerLib::registerArgHandler(ArgHandler* handler) {
  PacAssert(!handler->getName().empty(), "empty handler name");
  sgLogger.logMessage("register handler " + handler->getName(), SL_NORMAL);
  // check if it's already registerd
  ArgHandlerMap::iterator iter = std::find_if(mArgHandlerMap.begin(),
      mArgHandlerMap.end(), [&](ArgHandlerMap::value_type& v) -> bool {
        return v.first == handler->getName();
      });

  if (iter == mArgHandlerMap.end()) {
    mArgHandlerMap[handler->getName()] = handler;
  } else {
    PAC_EXCEPT(Exception::ERR_DUPLICATE_ITEM,
        handler->getName() + " already registed!");
  }
}

//------------------------------------------------------------------
void ArgHandlerLib::init() {
  this->registerArgHandler(new BlankArgHandler());
  this->registerArgHandler(new BoolArgHandler());
  // primitive decimal arg handlers
  this->registerArgHandler(new PriDeciArgHandler<short>("short"));
  this->registerArgHandler(new PriDeciArgHandler<unsigned short>("ushort"));
  this->registerArgHandler(new PriDeciArgHandler<int>("int"));
  this->registerArgHandler(new PriDeciArgHandler<unsigned int>("uint"));
  this->registerArgHandler(new PriDeciArgHandler<long>("long"));
  this->registerArgHandler(new PriDeciArgHandler<unsigned long>("ulong"));
  this->registerArgHandler(new PriDeciArgHandler<Real>("real"));
  // normalized real
  this->registerArgHandler(
      new PriDeciRangeArgHandler<Real>("nreal", -1.0, 1.0));
  // int 2..int5, real2..real5, matrix2, matrix3, matrix4
  this->registerArgHandler(sgArgLib.createMonoTree("int2", "int", 2));
  this->registerArgHandler(sgArgLib.createMonoTree("int3", "int", 3));
  this->registerArgHandler(sgArgLib.createMonoTree("int4", "int", 4));
  this->registerArgHandler(sgArgLib.createMonoTree("int5", "int", 5));
  this->registerArgHandler(sgArgLib.createMonoTree("real2", "real", 2));
  this->registerArgHandler(sgArgLib.createMonoTree("real3", "real", 3));
  this->registerArgHandler(sgArgLib.createMonoTree("real4", "real", 4));
  this->registerArgHandler(sgArgLib.createMonoTree("real5", "real", 5));
  this->registerArgHandler(sgArgLib.createMonoTree("nreal2", "nreal", 2));
  this->registerArgHandler(sgArgLib.createMonoTree("nreal3", "nreal", 3));
  this->registerArgHandler(sgArgLib.createMonoTree("nreal4", "nreal", 4));
  this->registerArgHandler(sgArgLib.createMonoTree("nreal5", "nreal", 5));
  this->registerArgHandler(sgArgLib.createMonoTree("matrix2", "real", 4));
  this->registerArgHandler(sgArgLib.createMonoTree("matrix3", "real", 9));
  this->registerArgHandler(sgArgLib.createMonoTree("matrix4", "real", 16));
  this->registerArgHandler(sgArgLib.createMonoTree("nmatrix2", "nreal", 4));
  this->registerArgHandler(sgArgLib.createMonoTree("nmatrix3", "nreal", 9));
  this->registerArgHandler(sgArgLib.createMonoTree("nmatrix4", "nreal", 16));

  // special handlers
  this->registerArgHandler(new PathArgHandler());
  this->registerArgHandler(new CmdArgHandler());
  this->registerArgHandler(new ParameterArgHandler());
  this->registerArgHandler(new ValueArgHandler());
}

//------------------------------------------------------------------
ArgHandler* ArgHandlerLib::createArgHandler(const std::string& protoName) {
  ArgHandlerMap::iterator iter = mArgHandlerMap.find(protoName);
  if (iter == mArgHandlerMap.end())
    PAC_EXCEPT(
        Exception::ERR_ITEM_NOT_FOUND, protoName + " not found in arg libs");
  return iter->second->clone();
}

//------------------------------------------------------------------
TreeArgHandler* ArgHandlerLib::createMonoTree(
    const std::string& name, const std::string& ahName, int num) {
  TreeArgHandler* tree = new TreeArgHandler(name);

  Node* node = tree->getRoot();
  for (int i = 0; i < num; ++i) {
    node = node->addChildNode(name + "_" + StringUtil::toString(i), ahName);
  }
  node->endBranch("0");
  return tree;
}

template <>
ArgHandlerLib* Singleton<ArgHandlerLib>::msSingleton = 0;
}
