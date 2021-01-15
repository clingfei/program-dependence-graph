#ifndef CALLWRAPPER_H_
#define CALLWRAPPER_H_
#include "LLVMEssentials.hh"
#include "Tree.hh"
#include "PDGUtils.hh"
#include "FunctionWrapper.hh"

namespace pdg
{
  class CallWrapper
  {
    private:
      llvm::CallInst* _call_inst;
      llvm::Function* _called_func;
      std::vector<llvm::Value *> _arg_list;
      std::map<llvm::Value *, Tree *> _arg_actual_in_tree_map;
      std::map<llvm::Value *, Tree *> _arg_actual_out_tree_map;
    public:
      CallWrapper(llvm::CallInst& ci)
      {
        assert(ci.getCalledFunction() != nullptr);
        _call_inst = &ci;
        _called_func = pdgutils::getCalledFunc(ci);
        for (auto arg_iter = ci.arg_begin(); arg_iter != ci.arg_end(); arg_iter++)
        {
          _arg_list.push_back(*arg_iter);
        }
      }
      void buildActualTreeForArgs(FunctionWrapper &callee_fw);
      llvm::CallInst *getCallInst() { return _call_inst; }
      llvm::Function *getCalledFunc() { return _called_func; }
      std::vector<llvm::Value *> &getArgList() { return _arg_list; }
      Tree *getArgActualInTree(llvm::Value &actual_arg);
      Tree *getArgActualOutTree(llvm::Value &actual_arg);
  };
}

#endif