#include "ASM/arm/ArmBuilder.hh"
#include <iostream>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/arm/RegAllocator.hh"
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
  auto cfg = std::make_shared<ControlFlowGraph>(current_, end_);
  reg_alloc_ = std::make_unique<RegAllocator>(LiveAnalyzer(cfg));
  //开头label包含了函数名
  auto func_label = (*current_)->a_;
  std::string func_name = func_label->get_name();
  //添加一个新函数在列表
  func_sections_.emplace_back(func_name);
  //绑定到body，后面简写
  auto *pfunc_section = &func_sections_.back().body_;
  [[maybe_unused]] auto emit = [pfunc_section](const std::string &inst) -> void { (*pfunc_section) += inst; };
  auto emitln = [pfunc_section](const std::string &inst) -> void {
    pfunc_section->append(inst);
    pfunc_section->append("\n");
  };
  //添加函数头
  emitln(".global " + func_name);
  emitln(func_name + ":");
  //将要用到的寄存器保存起来
  [[maybe_unused]] auto func_attr = reg_alloc_->get_SymAttribute(func_label);
  //
  for (int i = 4; i < 16; i++) {
  }

  reg_alloc_.release();
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
  return "_ref_" + name + ": .word " + name + "\n";
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler