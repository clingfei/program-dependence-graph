#include "SharedDataAnalysis.hh"

char pdg::SharedDataAnalysis::ID = 0;

using namespace llvm;

void pdg::SharedDataAnalysis::getAnalysisUsage(AnalysisUsage &AU) const
{
  AU.addRequired<ProgramDependencyGraph>();
  AU.setPreservesAll();
}

bool pdg::SharedDataAnalysis::runOnModule(llvm::Module &M)
{
  _module = &M;
  _PDG = getAnalysis<ProgramDependencyGraph>().getPDG();
  _call_graph = &PDGCallGraph::getInstance();
  fieldStatsFile.open("fieldAccessStats.csv");
  // read driver/kernel domian funcs
  setupStrOps();
  readSentinelFields();
  // insert driver global ops shared struct type
  readGlobalFuncOpStructNames();
  setupDriverFuncs(M);
  setupKernelFuncs(M);
  setupExportedFuncPtrFieldNames();
  // get boundary functions
  setupBoundaryFuncs(M);
  // compute struct type passed through boundary (shared struct type)
  readDriverGlobalStrucTypes();
  computeSharedStructDITypes();
  computeGlobalStructTypeNames();
  // build global tree for each struct type, connect with address variables
  buildTreesForSharedStructDIType(M);
  // generate shared field id
  computeSharedFieldID();
  // dumpSharedFieldID();
  errs() << "number of shared fields: " << _shared_field_id.size() << "\n";
  errs() << "number of kernel read driver update fields: " << _sharedKRDWFields << "\n";
  // computeSharedGlobalVars();
  if (!pdgutils::isFileExist("shared_struct_types"))
    dumpSharedTypes("shared_struct_types");
  // printPingPongCalls(M);
  // collect shared fields detail access stats
  collectSharedFieldsAccessStats();
  fieldStatsFile.close();
  errs() << "Shared fields computation finished\n";
  return false;
}

void pdg::SharedDataAnalysis::setupStrOps()
{
  _string_op_names.insert("strcpy");
  _string_op_names.insert("strncpy");
  _string_op_names.insert("strlen");
  _string_op_names.insert("strlcpy");
  _string_op_names.insert("strcmp");
  _string_op_names.insert("strchr");
  _string_op_names.insert("strncmp");
  _string_op_names.insert("strpbrk");
  _string_op_names.insert("kobject_set_name");
}

void pdg::SharedDataAnalysis::setupExportedFuncPtrFieldNames()
{
  std::ifstream driverExportedFuncPtrNames("exported_func_ptrs");
  for (std::string line; std::getline(driverExportedFuncPtrNames, line);)
  {
    exportedFuncPtrFieldNames.insert(line);
  }
}

void pdg::SharedDataAnalysis::setupDriverFuncs(Module &M)
{
  _driver_domain_funcs = readFuncsFromFile("driver_funcs", M);
}

void pdg::SharedDataAnalysis::readDriverGlobalStrucTypes()
{
  ifstream driver_global_struct_types("driver_global_struct_types");
  for (std::string line; std::getline(driver_global_struct_types, line);)
  {
    _driver_global_struct_types.insert(line);
  }
}

void pdg::SharedDataAnalysis::setupKernelFuncs(Module &M)
{
  for (auto &F : M)
  {
    if (F.isDeclaration() || F.empty())
      continue;
    // a special case is that driver function has module number concatenated, e.g., ixgbe_write_reg.5
    // neet to exclude such functions
    std::string func_name = F.getName().str();
    // ixgbe_write_reg.5 -> ixgbe_write_reg
    func_name = pdgutils::stripFuncNameVersionNumber(func_name);
    auto func = M.getFunction(StringRef(func_name));
    if (func == nullptr)
      continue;
    if (_driver_domain_funcs.find(func) == _driver_domain_funcs.end())
    {
      _kernel_domain_funcs.insert(func);
      _kernel_domain_func_names.insert(func->getName().str());
    }
  }
}

void pdg::SharedDataAnalysis::setupBoundaryFuncs(Module &M)
{
  auto imported_funcs = readFuncsFromFile("imported_funcs", M);
  auto exported_funcs = readFuncsFromFile("exported_funcs", M);

  if (EnableAnalysisStats)
  {
    auto &ksplit_stats = KSplitStats::getInstance();
    ksplit_stats._kernel_to_driver_func_call += exported_funcs.size();
    ksplit_stats._driver_to_kernel_func_call += imported_funcs.size();
  }

  _boundary_funcs.insert(imported_funcs.begin(), imported_funcs.end());
  _boundary_funcs.insert(exported_funcs.begin(), exported_funcs.end());
  // module init functions
  Function* init_func = getModuleInitFunc(M);
  if (init_func != nullptr)
    _boundary_funcs.insert(init_func);

  for (auto bf : _boundary_funcs)
  {
    _boundary_func_names.insert(bf->getName().str());
  }
}

std::set<Function *> pdg::SharedDataAnalysis::readFuncsFromFile(std::string file_name, Module &M)
{
  std::set<Function *> ret;
  std::ifstream ReadFile(file_name);
  for (std::string line; std::getline(ReadFile, line);)
  {
    Function *f = M.getFunction(StringRef(line));
    if (!f)
      continue;
    if (f->isDeclaration() || f->empty())
      continue;
    ret.insert(f);
  }
  return ret;
}

std::set<Function *> pdg::SharedDataAnalysis::computeBoundaryTransitiveClosure()
{
  std::set<Function*> boundary_trans_funcs;
  for (auto boundary_func : _boundary_funcs)
  {
    if (_call_graph->isExcludeFunc(*boundary_func))
    {
      errs() << "found exclude func " << boundary_func->getName() << "\n";
      continue;
    }
    auto func_node = _call_graph->getNode(*boundary_func);
    assert(func_node != nullptr && "cannot get function node for computing transitive closure!");
    auto trans_func_nodes = _call_graph->computeTransitiveClosure(*func_node);
    for (auto n : trans_func_nodes)
    {
      if (Function *trans_func = dyn_cast<Function>(n->getValue()))
        boundary_trans_funcs.insert(trans_func);
    }
  }
  return boundary_trans_funcs;
}
/*
Basically, we can both driver side and kernel side code. Then 
compute the intersaction of struct types used by both sides.
*/
void pdg::SharedDataAnalysis::computeSharedStructDITypes()
{
  std::set<std::string> driver_struct_type_names;
  for (auto func : _driver_domain_funcs)
  {
    if (func->isDeclaration())
      continue;
    for (auto inst_i = inst_begin(func); inst_i != inst_end(func); inst_i++)
    {
      Node *n = _PDG->getNode(*inst_i);
      if (!n)
        continue;
      DIType *node_di_type = n->getDIType();
      if (StoreInst *si = dyn_cast<StoreInst>(&*inst_i))
      {
        auto val_op = si->getValueOperand()->stripPointerCasts();
        Node *val_n = _PDG->getNode(*val_op);
        if (!val_n)
          continue;
        node_di_type = val_n->getDIType();
      }
      if (!node_di_type)
        continue;
      DIType *lowest_di_type = dbgutils::getLowestDIType(*node_di_type);
      if (lowest_di_type == nullptr)
        continue;
      if (dbgutils::isStructType(*lowest_di_type))
      {
        std::string di_type_name = dbgutils::getSourceLevelTypeName(*lowest_di_type, true);
        if (!di_type_name.empty())
          driver_struct_type_names.insert(pdgutils::stripVersionTag(di_type_name));
      }
    }
  }
  // errs() << " === driver side struct types ===\n";
  // for (auto t : driver_struct_type_names)
  // {
  //   errs() << "\t" << t << "\n";
  // }
  std::set<std::string> processed_struct_names;
  for (auto func : _kernel_domain_funcs)
  {
    if (func->isDeclaration())
      continue;
    for (auto inst_i = inst_begin(func); inst_i != inst_end(func); inst_i++)
    {
      Node *n = _PDG->getNode(*inst_i);
      if (!n)
        continue;
      DIType *node_di_type = n->getDIType();
      if (!node_di_type)
        continue;
      if (dbgutils::isStructType(*node_di_type) || dbgutils::isStructPointerType(*node_di_type))
      {
        DIType *lowest_di_type = dbgutils::getLowestDIType(*node_di_type);
        assert(lowest_di_type != nullptr && "null lowest di type (computeSharedStructTypes)\n");
        std::string struct_type_name = dbgutils::getSourceLevelTypeName(*lowest_di_type, true);
        struct_type_name = pdgutils::stripVersionTag(struct_type_name);
        if (processed_struct_names.find(struct_type_name) != processed_struct_names.end())
          continue;
        processed_struct_names.insert(struct_type_name);
        // check if driver side has the same di type, if so, add the type as shared type accessed by both driver and kernel
        if (driver_struct_type_names.find(struct_type_name) != driver_struct_type_names.end())
          _shared_struct_di_types.insert(lowest_di_type);
      }
    }
  }

  // for parameters passed across isolation boundary, if it's a struct type, then we need to consider this type as
  // shared data type. However, this steps seems unecessary, because the above step should include all the shared data types
  // computed in this step.
  // for (auto f : _boundary_funcs)
  // {
  //   if (f->isDeclaration())  
  //     continue;
  //   auto fw = _PDG->getFuncWrapper(*f);
  //   for (auto arg : fw->getArgList())
  //   {
  //     auto arg_di_type = fw->getArgDIType(*arg);
  //     auto arg_lowest_di_type = dbgutils::getLowestDIType(*arg_di_type);
  //     if (arg_lowest_di_type != nullptr)
  //     {
  //       if (dbgutils::isStructType(*arg_lowest_di_type))
  //         _shared_struct_di_types.insert(arg_lowest_di_type);
  //     }
  //   }

  //   auto ret_val_di_type = fw->getReturnValDIType();
  //   auto ret_val_lowest_di_type = dbgutils::getLowestDIType(*ret_val_di_type);
  //   if (ret_val_lowest_di_type != nullptr)
  //   {
  //     if (dbgutils::isStructType(*ret_val_lowest_di_type))
  //       _shared_struct_di_types.insert(ret_val_lowest_di_type);
  //   }
  // }
}

void pdg::SharedDataAnalysis::computeGlobalStructTypeNames()
{
  for (auto &global_var : _module->getGlobalList())
  {
    SmallVector<DIGlobalVariableExpression *, 4> sv;
    if (!global_var.hasInitializer())
      continue;
    DIGlobalVariable *di_gv = nullptr;
    global_var.getDebugInfo(sv);
    for (auto di_expr : sv)
    {
      if (di_expr->getVariable()->getName() == global_var.getName())
        di_gv = di_expr->getVariable(); // get global variable from global expression
    }
    if (di_gv == nullptr)
      continue;
    auto gv_di_type = di_gv->getType();
    if (gv_di_type == nullptr)
      continue;
    auto gv_lowest_di_type = dbgutils::getLowestDIType(*gv_di_type);
    if (gv_lowest_di_type == nullptr || gv_lowest_di_type->getTag() != dwarf::DW_TAG_structure_type)
      continue;
    _global_struct_di_type_names.insert("struct " + dbgutils::getSourceLevelTypeName(*gv_di_type, true));
  }
}

void pdg::SharedDataAnalysis::buildTreesForSharedStructDIType(Module &M)
{
  for (auto shared_struct_di_type : _shared_struct_di_types)
  {
    Tree *type_tree = new Tree();
    TreeNode *root_node = new TreeNode(shared_struct_di_type, 0, nullptr, type_tree, GraphNodeType::GLOBAL_TYPE);
    std::string shared_struct_type_name = dbgutils::getSourceLevelTypeName(*shared_struct_di_type, true);
    if (!shared_struct_type_name.empty())
      _shared_struct_type_names.insert(shared_struct_type_name);
    // this finds all the variable (global/local) in the code that have the struct type.
    // we will analylze the accesses to these variables to determine shared fields.
    auto vars_with_di_type = computeVarsWithDITypeInModule(*shared_struct_di_type, M);
    for (auto var : vars_with_di_type)
    {
      root_node->addAddrVar(*var);
    }
    type_tree->setRootNode(*root_node);
    type_tree->build(2);
    connectTypeTreeToAddrVars(*type_tree);
    _global_struct_di_type_map.insert(std::make_pair(shared_struct_di_type, type_tree));
  }
}

void pdg::SharedDataAnalysis::connectTypeTreeToAddrVars(Tree &type_tree)
{
  TreeNode *root_node = type_tree.getRootNode();
  std::queue<TreeNode *> node_queue;
  node_queue.push(root_node);
  while (!node_queue.empty())
  {
    TreeNode *current_node = node_queue.front();
    node_queue.pop();
    for (auto addr_var : current_node->getAddrVars())
    {
      if (!_PDG->hasNode(*addr_var))
        continue;
      auto addr_var_node = _PDG->getNode(*addr_var);
      current_node->addNeighbor(*addr_var_node, EdgeType::VAL_DEP);
      // consider aliasing
      auto aliasNodes = _PDG->findNodesReachedByEdge(*addr_var_node, EdgeType::DATA_ALIAS);
      for (auto aliasNode : aliasNodes)
      {
        current_node->addAddrVar(*aliasNode->getValue());
        current_node->addNeighbor(*aliasNode, EdgeType::VAL_DEP);
      }
    }
    for (auto child_node : current_node->getChildNodes())
    {
      node_queue.push(child_node);
    }
  }
}

void pdg::SharedDataAnalysis::computeVarsWithDITypeInFunc(DIType &dt, Function &F, std::set<Value *> &vars)
{
  for (auto inst_iter = inst_begin(F); inst_iter != inst_end(F); inst_iter++)
  {
    Node *inst_node = _PDG->getNode(*inst_iter);
    if (!inst_node)
      continue;
    DIType *inst_di_type = inst_node->getDIType();
    if (inst_di_type == nullptr)
      continue;
    // should also consider the pointer to the struct.
    DIType *inst_lowest_di_type = dbgutils::getLowestDIType(*inst_di_type);
    if (inst_lowest_di_type == nullptr)
      continue;
    if (dbgutils::hasSameDIName(dt, *inst_lowest_di_type))
    {
      vars.insert(&*inst_iter);
    }
  }
}

std::set<Value *> pdg::SharedDataAnalysis::computeVarsWithDITypeInModule(DIType &dt, Module &M)
{
  std::set<Value *> vars;
  for (auto &F : M)
  {
    if (F.isDeclaration() || F.empty())
      continue;
    computeVarsWithDITypeInFunc(dt, F, vars);
  }
  return vars;
}

bool pdg::SharedDataAnalysis::isStructFieldNode(TreeNode &tree_node)
{
  auto parent_node = tree_node.getParentNode();
  if (!parent_node)
    return false;
  auto parent_node_di_type = parent_node->getDIType();
  if (!parent_node_di_type)
    return false;
  return dbgutils::isStructType(*parent_node_di_type);
}

bool pdg::SharedDataAnalysis::isTreeNodeShared(TreeNode &tree_node, bool &hasReadByKernel, bool &hasUpdateByDriver)
{
  auto dt = tree_node.getDIType();
  if (dt && dbgutils::isFuncPointerType(*dt))
    return true;
  auto addr_vars = tree_node.getAddrVars();
  bool accessed_in_driver = false;
  bool accessed_in_kernel = false;

  for (auto addr_var : addr_vars)
  {
    if (Instruction *i = dyn_cast<Instruction>(addr_var))
    {
      Function *f = i->getFunction();

      if (_driver_domain_funcs.find(f) != _driver_domain_funcs.end())
      {
        accessed_in_driver = true;
        if (pdgutils::hasWriteAccess(*i))
          hasUpdateByDriver = true;
      }
      if (_kernel_domain_funcs.find(f) != _kernel_domain_funcs.end())
      {
        accessed_in_kernel = true;
        if (pdgutils::hasReadAccess(*i))
          hasReadByKernel = true;
      }
    }

    if (hasReadByKernel && hasUpdateByDriver)
    {
      _sharedKRDWFields++;
      return true;
    }
  }

  if (accessed_in_driver && accessed_in_kernel)
    return true;
  return false;
}

bool pdg::SharedDataAnalysis::isFieldUsedInStringOps(TreeNode &tree_node)
{
  std::set<EdgeType> edge_types = {EdgeType::PARAMETER_IN, EdgeType::DATA_ALIAS};
  auto reach_nodes = _PDG->findNodesReachedByEdges(tree_node, edge_types);
  for (auto node : reach_nodes)
  {
    if (node->getValue() != nullptr)
    {
      for (auto user : node->getValue()->users())
      {
        if (CallInst *ci = dyn_cast<CallInst>(user))
        {
          auto called_func = pdgutils::getCalledFunc(*ci);
          if (called_func == nullptr)
            continue;
          std::string called_func_name = called_func->getName().str();
          called_func_name = pdgutils::stripFuncNameVersionNumber(called_func_name);
          if (_string_op_names.find(called_func_name) != _string_op_names.end())
            return true;
        }
      }
    }
  }
  return false;
}

bool pdg::SharedDataAnalysis::isSharedFieldID(std::string field_id, std::string field_type_name)
{
  if (field_id.empty())
    return false;
  return (_shared_field_id.find(field_id) != _shared_field_id.end());
}

void pdg::SharedDataAnalysis::computeSharedFieldID()
{
  for (auto dt_tree_pair : _global_struct_di_type_map)
  {
    // auto di_type_name = dbgutils::getSourceLevelTypeName(*dt_tree_pair.first);
    // di_type_name = "struct." + di_type_name;
    // auto gb = _module->getNamedValue(StringRef(di_type_name));

    Tree *tree = dt_tree_pair.second;
    std::queue<TreeNode *> node_queue;
    node_queue.push(tree->getRootNode());
    // setting up map for collecting fields access stat
    auto sharedStructDIType = dbgutils::getLowestDIType(*tree->getRootNode()->getDIType());
    assert(sharedStructDIType != nullptr && "cannot collect fields access stats for empty struct di type\n");
    auto sharedStructName = dbgutils::getSourceLevelTypeName(*sharedStructDIType);

    while (!node_queue.empty())
    {
      TreeNode *current_tree_node = node_queue.front();
      node_queue.pop();
      DIType *nodeDIType = current_tree_node->getDIType();
      if (nodeDIType == nullptr)
        continue;

      bool isPtrType = dbgutils::isPointerType(*nodeDIType);
      bool isFuncPtrType = dbgutils::isFuncPointerType(*nodeDIType);
      bool hasReadByKernel = false;
      bool hasUpdateByDriver = false;
      if (current_tree_node->isStructField())
      {
        if (dbgutils::isFuncPointerType(*nodeDIType))
        {
          _shared_field_id.insert(pdgutils::computeTreeNodeID(*current_tree_node));
          countReadWriteAccessTimes(*current_tree_node);
        }
        else if (isTreeNodeShared(*current_tree_node, hasReadByKernel, hasUpdateByDriver))
        {
          auto fieldID = pdgutils::computeTreeNodeID(*current_tree_node);
          if (!fieldID.empty())
            _shared_field_id.insert(fieldID);
          // check whether a shared field is used in string operation.
          if (isFieldUsedInStringOps(*current_tree_node))
            _string_field_id.insert(fieldID);
          // check whether a field is used in pointer arithmetic
          if (DEBUG)
          {
            if (pdgutils::hasPtrArith(*current_tree_node, true))
              errs() << "find ptr arith in sd: " << fieldID << "\n";
          }
          // insert an entry for shared field access stat
          sharedStructTypeFieldsAccessMap[sharedStructName].insert(std::make_tuple(fieldID, isPtrType, isFuncPtrType, hasReadByKernel, hasUpdateByDriver));
          // collect read/write accesses in driver/kernel domain
          countReadWriteAccessTimes(*current_tree_node);
        }
      }

      for (auto child : current_tree_node->getChildNodes())
      {
        node_queue.push(child);
      }
    }
  }
  _shared_field_id.insert("struct inodei_rdev");
  _shared_field_id.insert("struct inodedevt");
  _shared_field_id.insert("struct filef_inode");
}

void pdg::SharedDataAnalysis::computeSharedGlobalVars()
{
  for (auto &global_var : _module->getGlobalList())
  {
    bool used_in_kernel = false;
    bool used_in_driver = false;
    for (auto user : global_var.users())
    {
      if (Instruction *i = dyn_cast<Instruction>(user))
      {
        auto func = i->getFunction();
        if (_kernel_domain_funcs.find(func) != _kernel_domain_funcs.end())
          used_in_kernel = true;
        else
          used_in_driver = true;
      }
      if (used_in_kernel && used_in_driver)
      {
        _shared_global_vars.insert(&global_var);
        break;
      }
    }
  }
}

void pdg::SharedDataAnalysis::dumpSharedFieldID()
{
  errs() << "dumping shared field id\n";
  for (auto id : _shared_field_id)
  {
    errs() << id << "\n";
  }
}

void pdg::SharedDataAnalysis::readSentinelFields()
{
  std::ifstream ReadFile("sentinel_fields");
  for (std::string line; std::getline(ReadFile, line);)
  {
    _sentinel_fields.insert(line);
  }
}

void pdg::SharedDataAnalysis::readGlobalFuncOpStructNames()
{
  std::ifstream ReadFile("global_op_struct_names");
  for (std::string line; std::getline(ReadFile, line);)
  {
    _driver_func_op_struct_names.insert(line);
    _shared_struct_type_names.insert(line);
  }
}

void pdg::SharedDataAnalysis::printPingPongCalls(Module &M)
{
  auto &call_g = PDGCallGraph::getInstance();
  if (!call_g.isBuild())
    call_g.build(M);
  unsigned cross_boundary_times = 0;
  for (auto f : _boundary_funcs)
  {
    if (f->isDeclaration())
      continue;
    auto caller_node = call_g.getNode(*f);
    if (!caller_node)
      continue;

    std::set<Function *> opposite_domain_funcs = _driver_domain_funcs;
    if (_driver_domain_funcs.find(f) != _driver_domain_funcs.end())
      opposite_domain_funcs = _kernel_domain_funcs;

    // check if this func can be called from the other domain.
    bool is_called = false;
    for (auto in_neighbor : caller_node->getInNeighbors())
    {
      auto node_val = in_neighbor->getValue();
      if (!isa<Function>(node_val))
        continue;
      Function *caller = cast<Function>(node_val);
      if (opposite_domain_funcs.find(caller) != opposite_domain_funcs.end())
      {
        is_called = true;
        break;
      }
    }

    if (!is_called)
      continue;

    std::queue<Node *> node_queue;
    std::unordered_set<Node *> seen_node;
    node_queue.push(caller_node);
    while (!node_queue.empty())
    {
      Node *n = node_queue.front();
      node_queue.pop();
      if (seen_node.find(n) != seen_node.end())
        continue;
      seen_node.insert(n);
      auto node_val = n->getValue();
      if (!isa<Function>(node_val))
        continue;
      Function *called_f = cast<Function>(node_val);
      if (opposite_domain_funcs.find(called_f) != opposite_domain_funcs.end())
      {
        cross_boundary_times++;
        break;
      }

      for (auto out_neighbor : n->getOutNeighbors())
      {
        node_queue.push(out_neighbor);
      }
    }
  }

  errs() << "================  ping pong call stats  ==================\n";
  errs() << "cross boundary times: " << cross_boundary_times << "\n";
  errs() << "num of boundary funcs: " << _boundary_funcs.size() << "\n";
  errs() << "==========================================================\n";
}

void pdg::SharedDataAnalysis::dumpSharedTypes(std::string file_name)
{
  std::ofstream output_file(file_name);
  for (auto shared_struct_type : _shared_struct_type_names)
  {
    if (!shared_struct_type.empty())
      output_file << shared_struct_type << "\n";
  }
  output_file.close();
}

Function *pdg::SharedDataAnalysis::getModuleInitFunc(Module &M)
{
  for (auto &F : M)
  {
    if (F.isDeclaration())
      continue;
    std::string func_name = F.getName().str();
    if (func_name.find("_init_module") != std::string::npos)
      return &F;
  }
  return nullptr;
}

void pdg::SharedDataAnalysis::collectSharedFieldsAccessStats()
{
  auto &ksplit_stats = KSplitStats::getInstance();
  ksplit_stats.numSharedStructType += sharedStructTypeFieldsAccessMap.size();

  for (auto iter = sharedStructTypeFieldsAccessMap.begin(); iter != sharedStructTypeFieldsAccessMap.end(); ++iter)
  {
    auto accessFieldStatTuples = iter->second;
    for (auto tupleIter = accessFieldStatTuples.begin(); tupleIter != accessFieldStatTuples.end(); ++tupleIter)
    {
      auto isPtrType = std::get<1>(*tupleIter);
      auto isFuncPtrType = std::get<2>(*tupleIter);
      auto isReadByKernel = std::get<3>(*tupleIter);
      auto isUpdateByDriver = std::get<4>(*tupleIter);
      if (isReadByKernel)
      {
        ksplit_stats.kernelReadableFieldsPerTy += 1;
        if (isPtrType)
          ksplit_stats.kernelReadablePtrFieldsPerTy += 1;
      }

      if (isUpdateByDriver)
      {
        ksplit_stats.driverWritableFieldsPerTy += 1;
        if (isPtrType)
        {
          errs() << "driver update ptr field (shared type): " << std::get<0>(*tupleIter) << "\n";
          ksplit_stats.driverWritablePtrFieldsPerTy += 1;
        }
      }

      if (isReadByKernel && isUpdateByDriver)
      {
        ksplit_stats.kernelReadDriverUpdateSharedFieldsPerTy += 1;
        if (isPtrType)
        {
          ksplit_stats.kernelReadDriverUpdateSharedPtrFieldsPerTy += 1;
          if (isFuncPtrType) 
            ksplit_stats.kernelReadDriverUpdateSharedFuncPtrFieldsPerTy += 1;
        }
      }
    }
  }
}

void pdg::SharedDataAnalysis::countReadWriteAccessTimes(TreeNode &treeNode)
{
  if (!treeNode.isStructField())
    return;

  auto addrVars = treeNode.getAddrVars();
  unsigned driverReadTimes = 0;
  unsigned driverWriteTimes = 0;
  unsigned kernelReadTimes = 0;
  unsigned kernelWriteTimes = 0;

  if (isFuncPtr(treeNode))
    driverWriteTimes += 1;

  for (auto addrVar : addrVars)
  {
    if (Instruction *i = dyn_cast<Instruction>(addrVar))
    {
      Function *f = i->getFunction();
      // count driver read write times
      if (_driver_domain_funcs.find(f) != _driver_domain_funcs.end())
      {
        if (pdgutils::hasWriteAccess(*i))
          driverWriteTimes++;
        else if (pdgutils::hasReadAccess(*i))
          driverReadTimes++;
      }
      // count kernel read write times
      else if (_kernel_domain_funcs.find(f) != _kernel_domain_funcs.end())
      {
        if (pdgutils::hasReadAccess(*i))
          kernelReadTimes++;
        else if (pdgutils::hasWriteAccess(*i))
          kernelWriteTimes++;
      }
    }
  }

  auto fieldName = treeNode.getSrcHierarchyName(false);
  if (kernelReadTimes != 0 && driverWriteTimes != 0) {
    std::string fieldTypeStr = getFieldTypeStr(treeNode);
    fieldStatsFile << fieldTypeStr << fieldName << "," << kernelReadTimes << "," << driverWriteTimes << "\n";
  }
}

std::string pdg::SharedDataAnalysis::getFieldTypeStr(TreeNode &treeNode)
{
  std::string ret = "";
  auto &ksplitStats = KSplitStats::getInstance();
  // first check branch conditions
  auto dt = treeNode.getDIType();
  if (dbgutils::isPointerType(*dt))
  {
    ret += "ptr|";
    ksplitStats.numRWPtr++;
    if (isFuncPtr(treeNode))
    {
      ksplitStats.numRWFuncPtr++;
      ret += "fp|";
    }
    if (usedInBranch(treeNode))
    {
      ksplitStats.numRWPtrCondVar++;
      ret += "cond|";
    }
    if (pdgutils::hasPtrArith(treeNode, true))
    {
      ksplitStats.numRWPtrInPtrArith++;
      ret += "ptrari|";
    }
  }
  else {
    ksplitStats.numRWNonPtr++;
    // non pointer type
    if (usedInBranch(treeNode))
    {
      ksplitStats.numRWNonPtrCondVar++;
      ret += "cond|";
    }
    if (pdgutils::hasPtrArith(treeNode, true))
    {
      ksplitStats.numRWNonPtrInPtrArith++;
      ret += "ptrari|";
    }
  }
  return ret;
}

bool pdg::SharedDataAnalysis::isFuncPtr(TreeNode &treeNode)
{
  auto dt = treeNode.getDIType();
  if (!dt)
    return false;
  if (dbgutils::isFuncPointerType(*dt))
    return true;
  return false;
}

bool pdg::SharedDataAnalysis::isDriverCallBackFuncPtrFieldNode(TreeNode &treeNode)
{
  auto dt = treeNode.getDIType();
  auto fieldName = treeNode.getSrcName();
  if (exportedFuncPtrFieldNames.find(fieldName) != exportedFuncPtrFieldNames.end())
    return true;
  return false;
}

bool pdg::SharedDataAnalysis::usedInBranch(TreeNode &treeNode)
{
  for (auto addrVar : treeNode.getAddrVars())
  {
    for (auto user : addrVar->users())
    {
      if (isa<BranchInst>(user))
        return true;
    }
  }
  return false;
}

static RegisterPass<pdg::SharedDataAnalysis>
    SharedDataAnalysis("shared-data", "Shared Data Analysis", false, true);