#include "Tree.hh"

using namespace llvm;

pdg::TreeNode::TreeNode(const TreeNode &tree_node) : Node(tree_node.getNodeType())
{
  _func = tree_node.getFunc();
  _di_type = tree_node.getDIType();
  _node_type = tree_node.getNodeType();
}

pdg::TreeNode::TreeNode(Function &f, DIType *di_type, int depth, TreeNode *parent_node, Tree *tree, GraphNodeType node_type) : Node(node_type)
{
  _di_type = di_type;
  _depth = depth;
  _parent_node = parent_node;
  _tree = tree;
  _func = &f;
}

int pdg::TreeNode::expandNode()
{
  // expand debugging information here
  if (_di_type == nullptr)
    return 0;
  // iterate through all the child nodes, build a tree node for each of them.
  if (!dbgutils::isPointerType(*_di_type) && !dbgutils::isStructType(*_di_type))
    return 0;

  if (dbgutils::isPointerType(*_di_type))
  {
    DIType* pointed_obj_dt = dbgutils::getLowestDIType(*_di_type);
    TreeNode *new_child_node = new TreeNode(*_func, pointed_obj_dt, _depth + 1, this, _tree, getNodeType());
    new_child_node->computeDerivedAddrVarsFromParent();
    _children.push_back(new_child_node);
    this->addNeighbor(*new_child_node, EdgeType::PARAMETER_FIELD);
    return 1;
  }
  // TODO: should change to aggregate type later
  if (dbgutils::isStructType(*_di_type))
  {
    auto di_node_arr = dyn_cast<DICompositeType>(_di_type)->getElements();
    for (unsigned i = 0; i < di_node_arr.size(); i++)
    {
      DIType *field_di_type = dyn_cast<DIType>(di_node_arr[i]);
      TreeNode *new_child_node = new TreeNode(*_func, field_di_type, _depth + 1, this, _tree, getNodeType());
      new_child_node->computeDerivedAddrVarsFromParent();
      _children.push_back(new_child_node);
      this->addNeighbor(*new_child_node, EdgeType::PARAMETER_FIELD);
    }
    return di_node_arr.size();
  }

  return 0;
}

void pdg::TreeNode::computeDerivedAddrVarsFromParent()
{
  if (_parent_node == nullptr)
    return;
  std::unordered_set<llvm::Value *> base_node_addr_vars;
  // handle struct pointer
  auto grand_parent_node = _parent_node->getParentNode();
  // TODO: now hanlde struct specifically, but should also verify on other aggregate pointer types
  if (dbgutils::isStructType(*_parent_node->getDIType()) && dbgutils::isStructPointerType(*grand_parent_node->getDIType()))
    base_node_addr_vars = grand_parent_node->getAddrVars();
  else
    base_node_addr_vars = _parent_node->getAddrVars();

  for (auto base_node_addr_var :base_node_addr_vars)
  {
    for (auto user : base_node_addr_var->users())
    {
      if (LoadInst* li = dyn_cast<LoadInst>(user))
      {
        _addr_vars.insert(li);
      }
      if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(user))
      {
        if (pdgutils::isGEPOffsetMatchDIOffset(*_di_type, *gep))
          _addr_vars.insert(gep);
      }
    }
  }
}

//  ====== Tree =======

pdg::Tree::Tree(const Tree& src_tree)
{
  TreeNode* src_tree_root_node = src_tree.getRootNode();
  TreeNode* new_root_node = new TreeNode(*src_tree_root_node);
  _root_node = new_root_node;
  _size = 0;
}

void pdg::Tree::print()
{
  std::queue<TreeNode*> node_queue;
  node_queue.push(_root_node);
  while (!node_queue.empty())
  {
    int queue_size = node_queue.size();
    while (queue_size > 0)
    {
      TreeNode *current_node = node_queue.front();
      node_queue.pop();
      queue_size--;
      if (current_node == _root_node)
        errs() << dbgutils::getSourceLevelVariableName(*current_node->getDILocalVar()) << ", ";
      else
      {
        if (current_node->getDIType() != nullptr)
          errs() << dbgutils::getSourceLevelVariableName(*current_node->getDIType()) << "(" << current_node->getAddrVars().size() << ")" << ", ";
      }
      for (auto child : current_node->getChildNodes())
      {
        node_queue.push(child);
      }
    }
    errs() << "\n";
  }
}

void pdg::Tree::build()
{
  int max_tree_depth = 5;
  int current_tree_depth = 0;
  std::queue<TreeNode *> node_queue;
  node_queue.push(_root_node);
  while (!node_queue.empty()) // have more child to expand
  {
    current_tree_depth++;
    if (current_tree_depth > max_tree_depth)
      break;
    int queue_size = node_queue.size();
    while (queue_size > 0)
    {
      queue_size--;
      TreeNode *current_node = node_queue.front();
      node_queue.pop();
      _size++;
      if (current_node->expandNode() > 0)
      {
        for (auto child_node : current_node->getChildNodes())
        {
          node_queue.push(child_node);
        }
      }
    }
  }
}