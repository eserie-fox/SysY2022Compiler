#include "ASM/arm/ArmBuilder.hh"
#include <algorithm>
#include <iostream>
#include <vector>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/arm/ArmHelper.hh"
#include "ASM/arm/RegAllocator.hh"
#include "MacroUtil.hh"
#include "MagicEnum.hh"
#include "TAC/TAC.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {
using namespace ThreeAddressCode;
ArmBuilder::ArmBuilder(TACListPtr tac_list) : tac_list_(tac_list) {}

bool ArmBuilder::AppendPrefix() { return true; }

bool ArmBuilder::AppendSuffix() { return true; }

bool ArmBuilder::TranslateGlobal() {
  current_ = tac_list_->begin();
  end_ = tac_list_->end();
  int func_level = 0;
  try {
    for (; current_ != end_; ++current_) {
      auto tac = *current_;
      switch (tac->operation_) {
        case TACOperationType::FunctionBegin:
          func_level++;
          break;
        case TACOperationType::FunctionEnd:
          func_level--;
          break;
        case TACOperationType::Label:
          break;
        case TACOperationType::Constant:
          if (!func_level) {
            if (tac->a_->type_ != SymbolType::Constant) {
              throw std::runtime_error("Expected constant in const declaration, but actually " +
                                       std::string(magic_enum::enum_name<SymbolType>(tac->a_->type_)));
            }
            if (tac->a_->value_.Type() != SymbolValue::ValueType::Array) {
              throw std::runtime_error(
                  "Only constant array allowed in const declaration, but actually " +
                  std::string(magic_enum::enum_name<SymbolValue::ValueType>(tac->a_->value_.Type())));
            }
            data_section_ += DeclareDataToASMString(tac);
            text_section_back_ += AddDataRefToASMString(tac);
          }
          break;
        case TACOperationType::Variable:
          if (!func_level) {
            if (tac->a_->type_ != SymbolType::Variable) {
              throw std::runtime_error("Expected variable in var declaration, but actually " +
                                       std::string(magic_enum::enum_name<SymbolType>(tac->a_->type_)));
            }
            if (!tac->a_->value_.IsNumericType() && tac->a_->value_.Type() != SymbolValue::ValueType::Array) {
              throw std::runtime_error(
                  "Expected array or int/float in var declaration, but actually " +
                  std::string(magic_enum::enum_name<SymbolValue::ValueType>(tac->a_->value_.Type())));
            }
            data_section_ += DeclareDataToASMString(tac);
            text_section_back_ += AddDataRefToASMString(tac);
          }
          break;
        default:
          if (!func_level) {
            init_section_ += GlobalTACToASMString(tac);
          }
          break;
      }
    }
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return false;
  }
  return true;
}

bool ArmBuilder::TranslateFunction() {
  //[current_,end_)区间内为即将处理的函数
  //拿到func_context_guard确保func_context拥有正确初始化和析构行为
  auto func_context_guard = ArmUtil::FunctionContextGuard(func_context_);
  //跳过fend
  --end_;
  {
    //解析寄存器分配
    auto cfg = std::make_shared<ControlFlowGraph>(current_, end_);
    func_context_.reg_alloc_ = new RegAllocator(LiveAnalyzer(cfg));
  }
  //开头label包含了函数名
  auto func_label = (*current_)->a_;
  std::string func_name = func_label->get_name();
  //添加一个新函数在列表
  func_sections_.emplace_back(func_name);
  //绑定到body，后面简写
  auto *pfunc_section = &func_sections_.back().body_;
  auto emit = [pfunc_section](const std::string &inst) -> void { (*pfunc_section) += inst; };
  auto emitln = [pfunc_section](const std::string &inst) -> void {
    pfunc_section->append(inst);
    pfunc_section->append("\n");
  };
  //添加函数头
  emitln(".global " + func_name);
  emitln(func_name + ":");
  //将要用到的寄存器保存起来
  auto func_attr = func_context_.reg_alloc_->get_SymAttribute(func_label);
  //保存了的寄存器列表
  std::vector<uint32_t> saveintregs, savefloatregs;

  //如果lr会被用到单独保存一下
  if (ISSET_UINT(func_attr.attr.used_regs.intRegs, LR_REGID)) {
    saveintregs.push_back(LR_REGID);
    func_context_.stack_size_for_regsave_ += 4;
  }
  //保存会修改的通用寄存器
  for (int i = 4; i < 13; i++) {
    //如果第i号通用寄存器要用
    if (ISSET_UINT(func_attr.attr.used_regs.intRegs, i)) {
      //那么保存它
      saveintregs.push_back(i);
      func_context_.stack_size_for_regsave_ += 4;
    }
  }
  //保存刚才确定好的通用寄存器
  {
    for (auto regid : saveintregs) {
      emitln("push { " + IntRegIDToName(regid) + " }");
    }
  }
  //保存浮点寄存器
  for (int i = 16; i < 32; i++) {
    if (ISSET_UINT(func_attr.attr.used_regs.floatRegs, i)) {
      savefloatregs.push_back(i);
      func_context_.stack_size_for_regsave_ += 4;
    }
  }
  //保存刚才确定好的浮点寄存器
  {
    for (auto regid : savefloatregs) {
      emitln("vpush { s" + std::to_string(regid) + " }");
    }
  }
  //为变量分配栈空间
  func_context_.stack_size_for_vars_ = func_attr.value;
  //可能用很大的栈空间，立即数存不下，保险起见Divide一下
  auto var_stack_immvals = ArmHelper::DivideIntoImmediateValues(func_context_.stack_size_for_vars_);
  for (auto immval : var_stack_immvals) {
    emitln("sub sp, sp, #" + std::to_string(immval));
  }

  //函数体翻译
  //跳过label和fbegin
  ++current_;
  ++current_;
  for (; current_ != end_; ++current_) {
    emit(FuncTACToASMString(*current_));
  }

  //释放变量栈空间
  std::reverse(var_stack_immvals.begin(), var_stack_immvals.end());
  for (auto immval : var_stack_immvals) {
    emitln("add sp, sp, #" + std::to_string(immval));
  }

  //还原寄存器
  //因栈的原因，还需要reverse一下
  //还原浮点寄存器
  {
    std::reverse(savefloatregs.begin(), savefloatregs.end());
    for (auto regid : savefloatregs) {
      emitln("vpop { s" + std::to_string(regid) + " }");
    }
  }
  //还原通用寄存器
  {
    std::reverse(saveintregs.begin(), saveintregs.end());
    for (auto regid : saveintregs) {
      emitln("pop { " + IntRegIDToName(regid) + " }");
    }
  }
  //这里没有用栈来pop lr到sp位置
  emitln("bx lr");

  return true;
}

bool ArmBuilder::TranslateFunctions() {
  int func_level = 0;
  //提取其中每一个函数
  TACList::iterator it = tac_list_->begin();
  TACList::iterator end = tac_list_->end();
  //记录函数头部位置
  TACList::iterator func_begin;
  for (; it != end; ++it) {
    if ((*it)->operation_ == TACOperationType::Label) {
      // label 后面接funcbegin标志着函数开始
      auto prev_it = it++;
      if (it != end_ && (*it)->operation_ == TACOperationType::FunctionBegin) {
        func_level++;
        assert(func_level < 2);
        //是函数，我们储存其开头位置
        func_begin = prev_it;
      } else {
        //不满足条件的话就不要让它多加一次了，进入下一次循环再处理
        current_ = prev_it;
      }
    } else if ((*it)->operation_ == TACOperationType::FunctionBegin) {
      //除非在第一种情况下，我们断言不会出现funcbegin，否则为错误
      throw std::runtime_error("Unexpected function begin");
    } else if ((*it)->operation_ == TACOperationType::FunctionEnd) {
      func_level--;
      assert(func_level >= 0);
      //找到函数末尾了
      current_ = func_begin;
      end_ = it;
      //因为是左闭右开，end_需要++
      ++end_;
      if (!TranslateFunction()) {
        return false;
      }
    }
  }

  return true;
}

bool ArmBuilder::Translate(std::string *output) {
  target_output_ = output;
  init_section_.clear();
  data_section_.clear();
  text_section_back_.clear();
  func_sections_.clear();

  if (!AppendPrefix()) {
    return false;
  }
  if (!TranslateGlobal()) {
    return false;
  }
  if (!TranslateFunctions()) {
    return false;
  }
  if (!AppendSuffix()) {
    return false;
  }
  return true;
}

std::string ArmBuilder::DeclareDataToASMString(TACPtr tac) {
  auto sym = tac->a_;
  //注释和align设置
  std::string ret = "// " + tac->ToString() + "\n.align 4\n";
  //数据label
  ret += sym->get_name() + ":\n";
  //如果是普通的int或float都是1单位sz，数组则可能多个
  size_t sz = 1;
  //数组声明
  if (sym->value_.Type() == SymbolValue::ValueType::Array) {
    sz = sym->value_.GetArrayDescriptor()->dimensions[0] * 4;
  }
  ret += ".skip " + std::to_string(sz) + "\n";
  return ret;
}

std::string ArmBuilder::AddDataRefToASMString(TACPtr tac) {
  std::string name = tac->a_->get_name();
  std::string ret = "// add reference to data '" + name + "'\n";
  ret += "_ref_" + name + ": .word " + name + "\n";
  return ret;
}

std::string ArmBuilder::IntRegIDToName(int regid) {
  if (0 <= regid && regid < 11) {
    return std::string("r") + std::to_string(regid);
  }
  if (regid == FP_REGID) {
    return "fp";
  }
  if (regid == IP_REGID) {
    return "ip";
  }
  if (regid == SP_REGID) {
    return "sp";
  }
  if (regid == LR_REGID) {
    return "lr";
  }
  if (regid == PC_REGID) {
    return "pc";
  }
  throw std::runtime_error("Unexpected regid " + std::to_string(regid));
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler