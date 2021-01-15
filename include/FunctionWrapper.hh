#ifndef FUNCTIONWRAPPER_H_
#define FUNCTIONWRAPPER_H_
#include "LLVMEssentials.hh"
#include "Tree.hh"
#include "PDGUtils.hh"

namespace pdg
{
  class FunctionWrapper
  {
  private:
    Node* _entry_node;
    llvm::Function *_func;
    std::vector<llvm::AllocaInst *> _alloca_insts;
    std::vector<llvm::DbgDeclareInst *> _dbg_declare_insts;
    std::vector<llvm::LoadInst *> _load_insts;
    std::vector<llvm::StoreInst *> _store_insts;
    std::vector<llvm::CallInst *> _call_insts;
    std::vector<llvm::ReturnInst *> _return_insts;
    std::vector<llvm::Argument *> _arg_list;
    std::map<llvm::Argument *, Tree *> _arg_formal_in_tree_map;
    std::map<llvm::Argument *, Tree *> _arg_formal_out_tree_map;
    std::pair<llvm::Value *, Tree *> _ret_val_formal_tree_pair;

  public:
    FunctionWrapper(llvm::Function *func)
    {
      _func = func;
      for (auto arg_iter = _func->arg_begin(); arg_iter != _func->arg_end(); arg_iter++)
      {
        _arg_list.push_back(&*arg_iter);
      }
      _entry_node = new Node(GraphNodeType::FUNC_ENTRY);
    }
    llvm::Function *getFunc() { return _func; }
    Node *getEntryNode() { return _entry_node; }
    void addInst(llvm::Instruction &i);
    void buildFormalTreeForArgs();
    llvm::DIType *getArgDIType(llvm::Argument &arg);
    llvm::DILocalVariable *getArgDILocalVar(llvm::Argument &arg);
    llvm::Value *getArgAllocaInst(llvm::Argument &arg);
    Tree *getArgFormalInTree(llvm::Argument &arg);
    Tree *getArgFormalOutTree(llvm::Argument &arg);
    std::map<llvm::Argument *, Tree *> &getArgFormalInTreeMap() { return _arg_formal_in_tree_map; }
    std::map<llvm::Argument *, Tree *> &getArgFormalOutTreeMap() { return _arg_formal_out_tree_map; }
    std::vector<llvm::AllocaInst *> &getAllocInsts() { return _alloca_insts; }
    std::vector<llvm::DbgDeclareInst *> &getDbgDeclareInsts() { return _dbg_declare_insts; }
    std::vector<llvm::LoadInst *> &getLoadInsts() { return _load_insts; }
    std::vector<llvm::StoreInst *> &getStoreInsts() { return _store_insts; }
    std::vector<llvm::CallInst *> &getCallInsts() { return _call_insts; }
    std::vector<llvm::ReturnInst *> &getReturnInsts() { return _return_insts; }
    std::vector<llvm::Argument *> &getArgList() { return _arg_list; }
  };
} // namespace pdg

#endif