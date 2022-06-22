#include "ASM/arm/ArmBuilder.hh"
#include <iostream>
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
        case TACOperationType::Constant: {
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
          break;
        }
        case TACOperationType::Variable: {
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
          break;
        }
        default:
          init_section_ += GlobalTACToASMString(tac);
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
  current_ = tac_list_->begin();
  end_ = tac_list_->end();
  int func_level = 0;

  for (; current_ != end_; ++current_) {
    if ((*current_)->operation_ == TACOperationType::Label) {
      // label 后面接funcbegin标志着函数开始
      auto prev_current = current_++;
      if (current_ != end_ && (*current_)->operation_ == TACOperationType::FunctionBegin) {
        func_level++;
        assert(func_level < 2);
        CreateNewFunc((*prev_current)->a_->get_name());
      } else {
        //不满足条件的话就不要让它多加一次了，进入下一次循环再处理
        current_ = prev_current;
      }
      continue;
    }
    //除非在第一种情况下，我们断言不会出现funcbegin，否则为错误
    if ((*current_)->operation_ == TACOperationType::FunctionBegin) {
      throw std::runtime_error("Unexpected function begin");
    }
    if ((*current_)->operation_ == TACOperationType::FunctionEnd) {
      func_level--;
      assert(func_level >= 0);
      continue;
    }
    //如果在函数中
    if (func_level) {
      func_sections_.back().body += FuncTACToASMString(*current_);
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
  if (!TranslateFunction()) {
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
  ret += "_data_" + sym->get_name() + ":\n";
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
  return "_ref_" + name + ": .word _data_" + name + "\n";
}

std::string ArmBuilder::GlobalTACToASMString([[maybe_unused]] TACPtr tac) { return ""; }

std::string ArmBuilder::FuncTACToASMString([[maybe_unused]] TACPtr tac) { return ""; }

void ArmBuilder::CreateNewFunc([[maybe_unused]] std::string func_name) {}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler