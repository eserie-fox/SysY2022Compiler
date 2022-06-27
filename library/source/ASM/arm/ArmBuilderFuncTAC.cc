#include <iostream>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/arm/ArmBuilder.hh"
#include "ASM/arm/ArmHelper.hh"
#include "ASM/arm/RegAllocator.hh"
#include "MagicEnum.hh"
#include "TAC/TAC.hh"

namespace HaveFunCompiler {
using namespace ThreeAddressCode;
namespace AssemblyBuilder {

std::string ArmBuilder::FuncTACToASMString([[maybe_unused]] TACPtr tac) {
  std::string ret = "";
  [[maybe_unused]] auto emitln = [&ret](const std::string &inst) -> void {
    ret.append(inst);
    ret.append("\n");
  };

  auto binary_operation = [&, this]() -> void {

  };
  auto unary_operation = [&, this]() -> void {

  };
  auto assignment = [&, this]() -> void {

  };
  auto branch = [&, this]() -> void {

  };
  auto parameter = [&, this]() -> void {
    auto sym = tac->a_;
    auto symAttr = func_context_.reg_alloc_->get_SymAttribute(sym);
    bool is_float;
    bool on_stack = false;
    int offset;
    //除Float类型外均为 int。（数组算指针同int）
    if (sym->value_.Type() == SymbolValue::ValueType::Float) {
      is_float = true;
      offset = func_context_.nfloat_param_++;
      if (offset >= 16) {
        on_stack = true;
        offset = func_context_.stack_size_for_params_;
        func_context_.stack_size_for_params_ += 4;
      }
    } else {
      is_float = false;
      offset = func_context_.nint_param_++;
      if (offset >= 4) {
        on_stack = true;
        offset = func_context_.stack_size_for_params_;
        func_context_.stack_size_for_params_ += 4;
      }
    }

    switch (symAttr.attr.store_type) {
      case symAttr.INT_REG: {
        if (is_float) {
          throw std::runtime_error("Store float value into int register");
        }
        if (!on_stack) {
          if (offset == 0) {
            if (symAttr.value != 0) {
              emitln("mov " + IntRegIDToName(symAttr.value) + ", r0");
            }
            break;
          } else if (symAttr.value != offset) {
            throw std::runtime_error("Move register parameter to another register");
          }
          break;
        }

        int32_t realoffset = func_context_.stack_size_for_vars_ + func_context_.stack_size_for_regsave_ + offset;
        if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
          emitln("ldr " + IntRegIDToName(symAttr.value) + ", [sp, #" + std::to_string(realoffset) + "]");
        } else {
          emitln("ldr " + IntRegIDToName(symAttr.value) + ", =" + std::to_string(realoffset));
          emitln("ldr " + IntRegIDToName(symAttr.value) + ", [sp, " + IntRegIDToName(symAttr.value) + "]");
        }
        break;
      }
      case symAttr.FLOAT_REG: {
        if (!is_float) {
          throw std::runtime_error("Store int value into float register");
        }
        if (!on_stack) {
          if (offset == 0) {
            if (symAttr.value != 0) {
              emitln("vmov s" + std::to_string(symAttr.value) + ", s0");
            }
            break;
          } else if (symAttr.value != offset) {
            throw std::runtime_error("Move register parameter to another register");
          }
          break;
        }
        int32_t realoffset = func_context_.stack_size_for_vars_ + func_context_.stack_size_for_regsave_ + offset;
        if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
          emitln("vldr s" + std::to_string(symAttr.value) + ", [sp, #" + std::to_string(realoffset) + "]");
        } else {
          int intReservedReg = func_context_.func_attr_.attr.used_regs.intReservedReg;
          emitln("ldr " + IntRegIDToName(intReservedReg) + ", =" + std::to_string(realoffset));
          emitln("add " + IntRegIDToName(intReservedReg) + ", " + IntRegIDToName(intReservedReg) + ", sp");
          emitln("vldr s" + std::to_string(symAttr.value) + ", [" + IntRegIDToName(intReservedReg) + "]");
        }
        break;
      }
      case symAttr.STACK_VAR: {
        if (on_stack) {
          throw std::runtime_error("Move stack parameter to another stack");
        }
        int32_t realoffset = symAttr.value;
        if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
          if (is_float) {
            emitln("vstr s" + std::to_string(offset) + ", [sp, #" + std::to_string(realoffset) + "]");
          } else {
            emitln("str " + IntRegIDToName(offset) + ", [sp, #" + std::to_string(realoffset) + "]");
          }
        } else {
          if (is_float) {
            int intReservedReg = func_context_.func_attr_.attr.used_regs.intReservedReg;
            emitln("ldr " + IntRegIDToName(intReservedReg) + ", =" + std::to_string(realoffset));
            emitln("add " + IntRegIDToName(intReservedReg) + ", " + IntRegIDToName(intReservedReg) + ", sp");
            emitln("vstr s" + std::to_string(offset) + ", [" + IntRegIDToName(intReservedReg) + "]");
          } else {
            emitln("ldr " + IntRegIDToName(offset) + ", =" + std::to_string(realoffset));
            emitln("ldr " + IntRegIDToName(offset) + ", [sp, " + IntRegIDToName(offset) + "]");
          }
        }
        break;
      }
      case symAttr.STACK_PARAM: {
        if (on_stack) {
          if (symAttr.value != offset) {
            throw std::runtime_error("Move stack parameter to another stack");
          }
          break;
        } else {
          throw std::runtime_error("Move register parameter to param stack");
        }
        break;
      }
      default:
        throw std::runtime_error("Unknown SymAttribute::StoreType");
        break;
    }
  };

  auto functionality = [&, this]() -> void {

  };

  switch (tac->operation_) {
    case TACOperationType::Add:
    case TACOperationType::Div:
    case TACOperationType::LessOrEqual:
    case TACOperationType::LessThan:
    case TACOperationType::Mod:
    case TACOperationType::Mul:
    case TACOperationType::NotEqual:
    case TACOperationType::Sub:
    case TACOperationType::GreaterOrEqual:
    case TACOperationType::GreaterThan:
    case TACOperationType::Equal:
      func_context_.parameter_head_ = false;
      binary_operation();
      break;
    case TACOperationType::UnaryMinus:
    case TACOperationType::UnaryPositive:
    case TACOperationType::UnaryNot:
      func_context_.parameter_head_ = false;
      unary_operation();
      break;
    case TACOperationType::Assign:
      func_context_.parameter_head_ = false;
      assignment();
      break;
    case TACOperationType::Goto:
    case TACOperationType::IfZero:
      func_context_.parameter_head_ = false;
      branch();
      break;
    case TACOperationType::Parameter:
      if (!func_context_.parameter_head_) {
        throw std::runtime_error("Parameters should be the head of function.");
      }
      parameter();
      break;
    case TACOperationType::Argument:
    case TACOperationType::ArgumentAddress:
    case TACOperationType::Call:
    case TACOperationType::Return:
      func_context_.parameter_head_ = false;
      functionality();
      break;
    case TACOperationType::Label:
    case TACOperationType::Variable:
    case TACOperationType::Constant:
    case TACOperationType::BlockBegin:
    case TACOperationType::BlockEnd:
      func_context_.parameter_head_ = false;
      // ignore
      break;
    default:
      throw std::runtime_error("Unexpected operation: " +
                               std::string(magic_enum::enum_name<TACOperationType>(tac->operation_)));
      break;
  }

  return ret;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler