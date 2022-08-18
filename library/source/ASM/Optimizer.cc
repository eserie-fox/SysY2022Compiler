#include "ASM/Optimizer.hh"
#include <vector>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/DataFlowManager.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/SymAnalyzer.hh"
#include "ASM/ArrivalAnalyzer.hh"
#include "TAC/Symbol.hh"
#include "ASM/CFG2DomTreeBuilder.hh"
#include "DomTreeHelper.hh"
#include <iostream>
#include <cassert>

namespace HaveFunCompiler {
namespace AssemblyBuilder {

using namespace ThreeAddressCode;

OptimizeController_Simple::OptimizeController_Simple(TACListPtr tacList, TACList::iterator fbegin,
                                                     TACList::iterator fend)
{
  simpleOp = std::make_shared<SimpleOptimizer>(tacList, fbegin, fend);
  PropagationOp = std::make_shared<PropagationOptimizer>(tacList, fbegin, fend);
  deadCodeOp = std::make_shared<DeadCodeOptimizer>(tacList, fbegin, fend);
  constFoldOp = std::make_shared<ConstantFoldingOptimizer>(tacList, fbegin, fend);
}

void OptimizeController_Simple::doOptimize()
{
  size_t round = 0;
  ssize_t cnt = 0;
  do
  {
    cnt = 0;
    cnt += simpleOp->optimize();
    cnt += PropagationOp->optimize();
    cnt += deadCodeOp->optimize();
    cnt += constFoldOp->optimize();
    ++round;
  } while (round < MAX_ROUND && cnt > MIN_OP_THRESHOLD);
  constFoldOp->optimize();   // 翻译时不能出现未折叠的常量运算
}

DeadCodeOptimizer::DeadCodeOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend)
    : _fbegin(fbegin), _fend(fend), _tacls(tacList) {}

int DeadCodeOptimizer::optimize() {
  cfg = std::make_shared<ControlFlowGraph>(_fbegin, _fend);
  liveAnalyzer = std::make_shared<LiveAnalyzer>(cfg);
  liveAnalyzer->analyze();

  std::vector<TACList::iterator> deadCodes;

  FOR_EACH_NODE(i, cfg) {
    auto tac = cfg->get_node_tac(i);
    auto defSym = tac->getDefineSym();
    if (defSym) {
      if (liveAnalyzer->isOutLive(defSym, i) && !hasSideEffect(defSym, tac)) 
        deadCodes.push_back(cfg->get_node_itr(i));
    }
  }

  for (auto it : deadCodes) 
    _tacls->erase(it);

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

DeadCodeOptimizer_UseDef::DeadCodeOptimizer_UseDef(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend) :
_fbegin(fbegin), _fend(fend), _tacls(tacList) { }

bool DeadCodeOptimizer_UseDef::hasSideEffect(SymbolPtr defSym, TACPtr tac) {
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

int DeadCodeOptimizer_UseDef::optimize()
{
  cfg = std::make_shared<ControlFlowGraph>(_fbegin, _fend);
  useDefAnalyzer = std::make_shared<UseDefAnalyzer>(cfg);
  useDefAnalyzer->analyze();

  std::vector<TACList::iterator> deadCodes;
  auto &symSet = useDefAnalyzer->get_symSet();
  for (auto sym : symSet)
  {
    auto &duChain = useDefAnalyzer->get_defUseChain(sym);
    for (auto& [n, uses] : duChain)
    {
      if (uses.size() == 0 && !hasSideEffect(sym, cfg->get_node_tac(n)))
        deadCodes.push_back(cfg->get_node_itr(n));
    }
  }

  for (auto it : deadCodes) 
    _tacls->erase(it);

  return deadCodes.size();
}


PropagationOptimizer::PropagationOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend) :
  tacLs_(tacList), fbegin_(fbegin), fend_(fend) {}

int PropagationOptimizer::optimize()
{
  auto isVarAssign = [](TACPtr tac)
  {
    return tac->operation_ == TACOperationType::Assign && tac->b_->value_.IsNumericType();
  };

  auto replace = [](TACPtr tac, SymbolPtr oldSym, SymbolPtr newSym)
  {
    // 提取tac中的使用变量指针
    SymbolPtr* arr[3] = {nullptr};
    size_t i = 0;
    if (tac->a_)
    {
      if (tac->a_->value_.Type() == SymbolValue::ValueType::Array)
        arr[i++] = &(tac->a_->value_.GetArrayDescriptor()->base_offset);
      else
        arr[i++] = &tac->a_;
    }
    if (tac->b_)
    {
      if (tac->b_->value_.Type() == SymbolValue::ValueType::Array)
        arr[i++] = &(tac->b_->value_.GetArrayDescriptor()->base_offset);
      else
        arr[i++] = &tac->b_;
    }
    if (tac->c_)
      arr[i++] = &tac->c_;

    for (auto p : arr)
      if (p && *p == oldSym)
        *p = newSym;
  };

  cfg = std::make_shared<ControlFlowGraph>(fbegin_, fend_);
  arrivalValAnalyzer = std::make_shared<ArrivalAnalyzer>(cfg);
  arrivalValAnalyzer->analyze();
  arrivalValAnalyzer->updateUseDefChain();

  auto domTree = BuildDomTreeFromCFG(cfg);
  DomTreeHelper domTreeHelper(domTree);

  auto symAnalyzer = arrivalValAnalyzer->get_symAnalyzer();
  auto &useDefChain = arrivalValAnalyzer->get_useDefChain();
  int cnt = 0;

  FOR_EACH_NODE(n, cfg)
  {
    auto tac = cfg->get_node_tac(n);
    auto useSyms = tac->getUseSym();
    for (auto sym : useSyms)
    {
      // 全局变量和字面量，无法进行传播
      if (sym->IsGlobal() || sym->IsLiteral())
        continue;

      auto &arrivalDefs = useDefChain.at(sym).at(n);

      // 能到达的只有1个定值
      if (arrivalDefs.size() == 1)
      {
        auto defNode = *arrivalDefs.begin();

        // 获取定值tac
        auto defTac = cfg->get_node_tac(defNode);

        // 正确性 DEBUG用
        assert(defTac->a_ == sym);

        // 当该定值是单赋值(a = b)形式时，尝试进行传播，无论b是字面量或单变量都可以
        if (isVarAssign(defTac))
        {
          // 如果b是一个常量，直接传播
          if (defTac->b_->IsLiteral())
          {
            replace(tac, sym, defTac->b_);
            ++cnt;
          }

          // 如果b是一个变量，必须保证它在n可用
          // 可用性条件：
          // 到达n和defNode的，变量b的定值，必须完全相同，否则在n点，a的值不一定和b相同
          // defNode，必须是n的必经结点，否则运行到n点时，程序不一定经过了defNode，b的值也不一定和a的相同
          else
          {
            // 得到到达结点node的变量sym的定值集合
            auto getSymReachableDefs = [this, &symAnalyzer](SymbolPtr sym, size_t node)
            {
              auto &rDefs = arrivalValAnalyzer->getIn(node);
              auto &symDefs = symAnalyzer->getSymDefPoints(sym);
              std::unordered_set<size_t> res;
              for (auto d : symDefs)
                if (rDefs.test(d))
                  res.insert(d);
              return res;
            };

            // defNode不是n的必经结点，则不能传播
            if (domTreeHelper.IsAncestorOf(defNode, n) == false)
              continue;

            auto bDefsAtDefNode = getSymReachableDefs(defTac->b_, defNode);
            auto bDefsAtn = getSymReachableDefs(defTac->b_, n);

            // 如果定值集合相等，可以进行传播
            if (bDefsAtDefNode == bDefsAtn)
            {
              replace(tac, sym, defTac->b_);
              ++cnt;
            }
          }
        }
      }
    }
  }
  return cnt;
}

SimpleOptimizer::SimpleOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend)
    : fbegin_(fbegin), fend_(fend), tacls_(tacList) {}

int SimpleOptimizer::optimize() {
  auto it = fbegin_;
  std::vector<decltype(it)> rmls;
  int opt_count = 0;
  // a = b;
  auto change2assign = [&opt_count](HaveFunCompiler::ThreeAddressCode::ThreeAddressCode &tac, SymbolPtr a,
                                    SymbolPtr b) -> void {
    tac.operation_ = TACOperationType::Assign;
    tac.a_ = a;
    tac.b_ = b;
    tac.c_ = nullptr;
    opt_count++;
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
        if (tac.b_->IsLiteral()) {
          if (iszero(tac.b_)) {
            change2assign(tac, tac.a_, tac.c_);
            break;
          }
        }
        if (tac.c_->IsLiteral()) {
          if (iszero(tac.c_)) {
            change2assign(tac, tac.a_, tac.b_);
            break;
          }
        }
        break;
      }

      case TACOperationType::Sub: {
        if (tac.c_->IsLiteral()) {
          if (iszero(tac.c_)) {
            change2assign(tac, tac.a_, tac.b_);
            break;
          }
        }
        break;
      }

      case TACOperationType::Mul: {
        if (tac.b_->IsLiteral()) {
          if (isone(tac.b_)) {
            change2assign(tac, tac.a_, tac.c_);
            break;
          }
          if (iszero(tac.b_)) {
            change2assign(tac, tac.a_, tac.b_);
            break;
          }
        }
        if (tac.c_->IsLiteral()) {
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
        if (tac.b_->IsLiteral()) {
          if (iszero(tac.b_)) {
            change2assign(tac, tac.a_, tac.b_);
            break;
          }
        }
        if (tac.c_->IsLiteral()) {
          if (isone(tac.c_)) {
            change2assign(tac, tac.a_, tac.b_);
            break;
          }
        }
        break;
      }

      case TACOperationType::Mod: {
        if (tac.b_->IsLiteral()) {
          if (iszero(tac.b_)) {
            change2assign(tac, tac.a_, tac.b_);
            break;
          }
        }
        if (tac.c_->IsLiteral()) {
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

      case TACOperationType::IfZero: {
        if (tac.b_->IsLiteral()) {
          if (iszero(tac.b_)) {
            tac.operation_ = TACOperationType::Goto;
            tac.b_ = nullptr;
          } else {
            rmls.push_back(it);
          }
          opt_count++;
        }
        break;
      }

      default:
        break;
    }
    if (tac.operation_ == TACOperationType::Assign) {
      if (tac.a_ == tac.b_) {
        rmls.push_back(it);
        opt_count++;
      }
    }
    ++it;
  }
  for (auto tac_it : rmls) {
    tacls_->erase(tac_it);
  }
  return opt_count;
}
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler