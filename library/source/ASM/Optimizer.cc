#include "ASM/Optimizer.hh"
#include <vector>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/DataFlowManager.hh"
#include "ASM/LiveAnalyzer.hh"
#include "TAC/Symbol.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

using namespace ThreeAddressCode;

OptimizeController_Simple::OptimizeController_Simple(TACListPtr tacList, TACList::iterator fbegin,
                                                     TACList::iterator fend)
    : tacls_(tacList) {
  dataFlowManager = std::make_shared<DataFlowManager_simple>(fbegin, fend);
  deadCodeOp = std::make_shared<DeadCodeOptimizer>(dataFlowManager, tacList);
}

void OptimizeController_Simple::doOptimize() { deadCodeOp->optimize(); }

DeadCodeOptimizer::DeadCodeOptimizer(std::shared_ptr<DataFlowManager> dataFlowManager, TACListPtr tacList)
    : _tacls(tacList), dfm(dataFlowManager) {
  _fbegin = dfm->get_fbegin();
  _fend = dfm->get_fend();
}

int DeadCodeOptimizer::optimize() {
  auto cfg = dfm->get_controlFlowGraph();
  auto liveAnalyzer = dfm->get_liveAnalyzer();

  std::vector<TACList::iterator> deadCodes;

  auto tacNum = cfg->get_nodes_number();
  for (size_t i = 0; i < tacNum; ++i) {
    auto tac = cfg->get_node_tac(i);
    auto defSym = tac->getDefineSym();
    if (defSym) {
      auto &outLive = liveAnalyzer->getOut(i);
      if (outLive.count(defSym) == 0 && !hasSideEffect(defSym, tac)) {
        deadCodes.push_back(cfg->get_node_itr(i));
        dfm->remove(i);
      }
    }
  }

  for (auto it : deadCodes) _tacls->erase(it);

  return deadCodes.size();
}

bool DeadCodeOptimizer::hasSideEffect(SymbolPtr defSym, TACPtr tac) {
  if (defSym->IsGlobal()) return true;
  if (tac->operation_ == TACOperationType::Call || tac->operation_ == TACOperationType::Parameter) return true;
  // if (tac->operation_ == TACOperationType::Constant || tac->operation_ == TACOperationType::Variable)
  // {
  //     auto liveInfo = la.get_symLiveInfo(defSym);
  //     if (liveInfo->useCnt != 0)
  //         return true;
  //     else
  //         return false;
  // }
  return false;
}

SimpleOptimizer::SimpleOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend)
    : fbegin_(fbegin), fend_(fend), tacls_(tacList) {}

void SimpleOptimizer::optimize() {
  auto it = fbegin_;
  std::vector<decltype(it)> rmls;
  // a = b;
  auto change2assign = [](HaveFunCompiler::ThreeAddressCode::ThreeAddressCode &tac, SymbolPtr a, SymbolPtr b) -> void {
    tac.operation_ = TACOperationType::Assign;
    tac.a_ = a;
    tac.b_ = b;
    tac.c_ = nullptr;
  };
  auto iszero = [](SymbolPtr sym) -> bool {
    if (sym->value_.UnderlyingType() == SymbolValue::ValueType::Float) {
      if (sym->value_.GetFloat() == 0) {
        return true;
      }
    } else {
      if (sym->value_.GetInt() == 0) {
        return true;
      }
    }
    return false;
  };
  auto isone = [](SymbolPtr sym) -> bool {
    if (sym->value_.UnderlyingType() == SymbolValue::ValueType::Float) {
      if (sym->value_.GetFloat() == 1) {
        return true;
      }
    } else {
      if (sym->value_.GetInt() == 1) {
        return true;
      }
    }
    return false;
  };
  while (it != fend_) {
    auto &tac = *(*it);
    switch (tac.operation_) {
      case TACOperationType::Add: {
        if (tac.b_->type_ == SymbolType::Constant) {
          if (iszero(tac.b_)) {
            change2assign(tac, tac.a_, tac.c_);
            break;
          }
        }
        if (tac.c_->type_ == SymbolType::Constant) {
          if (iszero(tac.c_)) {
            change2assign(tac, tac.a_, tac.b_);
            break;
          }
        }
        break;
      }
      case TACOperationType::Sub: {
        if (tac.c_->type_ == SymbolType::Constant) {
          if (iszero(tac.c_)) {
            change2assign(tac, tac.a_, tac.b_);
            break;
          }
        }
        break;
      }
      case TACOperationType::Mul: {
        if (tac.b_->type_ == SymbolType::Constant) {
          if (isone(tac.b_)) {
            change2assign(tac, tac.a_, tac.c_);
            break;
          }
          if (iszero(tac.b_)) {
            change2assign(tac, tac.a_, tac.b_);
            break;
          }
        }
        if (tac.c_->type_ == SymbolType::Constant) {
          if (isone(tac.c_)) {
            change2assign(tac, tac.a_, tac.b_);
            break;
          }
          if (iszero(tac.c_)) {
            change2assign(tac, tac.a_, tac.c_);
            break;
          }
        }
        break;
      }

      case TACOperationType::Div: {
        if (tac.b_->type_ == SymbolType::Constant) {
          if (iszero(tac.b_)) {
            change2assign(tac, tac.a_, tac.b_);
            break;
          }
        }
        if (tac.c_->type_ == SymbolType::Constant) {
          if (isone(tac.c_)) {
            change2assign(tac, tac.a_, tac.b_);
            break;
          }
        }
        break;
      }

      case TACOperationType::Mod: {
        if (tac.b_->type_ == SymbolType::Constant) {
          if (iszero(tac.b_)) {
            change2assign(tac, tac.a_, tac.b_);
            break;
          }
        }
        if (tac.c_->type_ == SymbolType::Constant) {
          if (isone(tac.c_)) {
            if (tac.a_->value_.Type() != SymbolValue::ValueType::Float) {
              auto sym = std::make_shared<Symbol>();
              sym->type_ = SymbolType::Constant;
              sym->value_ = SymbolValue(0);
              change2assign(tac, tac.a_, sym);
            }
            break;
          }
        }
        break;
      }
      default:
        break;
    }
    if (tac.operation_ == TACOperationType::Assign) {
      if (tac.a_ == tac.b_) {
        rmls.push_back(it);
      }
    }
    ++it;
  }
  for (auto tac_it : rmls) {
    tacls_->erase(tac_it);
  }
}
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler