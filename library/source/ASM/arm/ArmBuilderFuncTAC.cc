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

std::string ArmBuilder::FuncTACToASMString(TACPtr tac) {
  std::string ret = "";
  auto emitln = [&ret](const std::string &inst) -> void {
    ret.append(inst);
    ret.append("\n");
  };

  //获得相对于当前sp的地址
  auto getrealoffset = [&, this](SymAttribute &attr) -> int32_t {
    if (attr.attr.store_type == attr.STACK_VAR) {
      //定位到变量区
      return attr.value + func_context_.stack_size_for_args_;
    } else {
      //定位到形参区
      return attr.value + func_context_.stack_size_for_args_ + func_context_.stack_size_for_vars_ +
             func_context_.stack_size_for_regsave_;
    }
  };

  // reg_id为0时驱逐int_freereg1，否则驱逐int_freereg2。
  auto evit_int_reg = [&, this](int reg_id) -> void {
    SymbolPtr *target_sym;
    if (reg_id) {
      target_sym = &func_context_.int_freereg2_;
      reg_id = func_context_.func_attr_.attr.used_regs.intReservedReg;
    }
    target_sym = &func_context_.int_freereg1_;
    if (*target_sym == nullptr) {
      //不需要驱逐
      return;
    }
    if ((*target_sym)->IsLiteral()) {
      //字面量不用回存。
      *target_sym = nullptr;
      return;
    }
    auto attr = func_context_.reg_alloc_->get_SymAttribute(*target_sym);
    if (attr.attr.store_type == attr.INT_REG || attr.attr.store_type == attr.FLOAT_REG) {
      throw std::logic_error("Cannot evit register variable");
    }

    int32_t realoffset = getrealoffset(attr);

    if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
      emitln("str " + IntRegIDToName(reg_id) + ", [sp, #" + std::to_string(realoffset) + "]");
    } else {
      emitln("ldr " + IntRegIDToName(reg_id) + ", =" + std::to_string(realoffset));
      emitln("ldr " + IntRegIDToName(reg_id) + ", [sp, " + IntRegIDToName(reg_id) + "]");
    }
    *target_sym = nullptr;
  };

  // 自动选择驱逐一个不常用的freereg，并返回其编号
  auto get_free_int_reg = [&, this]() -> int {
    if (func_context_.int_freereg1_ == nullptr) {
      return 0;
    }
    if (func_context_.int_freereg2_ == nullptr) {
      return func_context_.func_attr_.attr.used_regs.intReservedReg;
    }
    //如果上一个在用int_freereg1，则驱逐reg2
    if (func_context_.last_int_freereg_ != 0) {
      int reg_id = func_context_.func_attr_.attr.used_regs.intReservedReg;
      evit_int_reg(reg_id);
      return reg_id;
    } else {
      evit_int_reg(0);
      return 0;
    }
  };

  // reg_id为0时驱逐float_freereg1，否则驱逐float_freereg2。
  auto evit_float_reg = [&, this](int reg_id) -> void {
    SymbolPtr *target_sym;
    if (reg_id) {
      target_sym = &func_context_.float_freereg2_;
      reg_id = func_context_.func_attr_.attr.used_regs.floatRegs;
    }
    target_sym = &func_context_.float_freereg1_;
    if (*target_sym == nullptr) {
      //不需要驱逐
      return;
    }
    if ((*target_sym)->IsLiteral()) {
      //字面量不用回存。
      *target_sym = nullptr;
      return;
    }
    auto attr = func_context_.reg_alloc_->get_SymAttribute(*target_sym);
    if (attr.attr.store_type == attr.INT_REG || attr.attr.store_type == attr.FLOAT_REG) {
      throw std::logic_error("Cannot evit register variable");
    }

    int32_t realoffset = getrealoffset(attr);

    if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
      emitln("vstr s" + std::to_string(reg_id) + ", [sp, #" + std::to_string(realoffset) + "]");
    } else {
      int freeintreg = get_free_int_reg();
      emitln("ldr " + IntRegIDToName(freeintreg) + ", =" + std::to_string(realoffset));
      emitln("add " + IntRegIDToName(freeintreg) + ", " + IntRegIDToName(freeintreg) + ", sp");
      emitln("vstr s" + std::to_string(reg_id) + ", [" + IntRegIDToName(freeintreg) + "]");
    }
    *target_sym = nullptr;
  };

  // 自动选择驱逐一个不常用的freereg，并返回其编号
  [[maybe_unused]] auto get_free_float_reg = [&, this]() -> int {
    if (func_context_.float_freereg1_ == nullptr) {
      return 0;
    }
    if (func_context_.float_freereg2_ == nullptr) {
      return func_context_.func_attr_.attr.used_regs.floatReservedReg;
    }
    //如果上一个在用float_freereg1，则驱逐reg2
    if (func_context_.last_float_freereg_ != 0) {
      int reg_id = func_context_.func_attr_.attr.used_regs.floatReservedReg;
      evit_float_reg(reg_id);
      return reg_id;
    } else {
      evit_float_reg(0);
      return 0;
    }
  };

  //分配除了except_reg之外的一个reg。但若reg_alloc有指示，或者已经在寄存器中缓存，则无视except_reg
  auto alloc_reg = [&, this](SymbolPtr sym, int except_reg = -1) -> int {
    //如果有缓存就直接用
    bool is_float = (sym->value_.Type() == SymbolValue::ValueType::Float);

    if (is_float) {
      if (func_context_.float_freereg1_ == sym) {
        return 0;
      }
      if (func_context_.float_freereg2_ == sym) {
        return func_context_.func_attr_.attr.used_regs.floatReservedReg;
      }
    } else {
      if (func_context_.int_freereg1_ == sym) {
        return 0;
      }
      if (func_context_.int_freereg2_ == sym) {
        return func_context_.func_attr_.attr.used_regs.intReservedReg;
      }
    }

    if(!sym->IsLiteral()){
      auto attr = func_context_.reg_alloc_->get_SymAttribute(sym);
      if (attr.attr.store_type == attr.FLOAT_REG || attr.attr.store_type == attr.INT_REG) {
        // reg_alloc已经有指示了，直接用就好了。
        return attr.value;
      }
    }
    

    //如果没缓存
    if (is_float) {
      //尽量选择一个既不是经常被用到，又不是except_reg的reg
      int target_reg = func_context_.last_float_freereg_ ? 0 : func_context_.func_attr_.attr.used_regs.floatReservedReg;
      SymbolPtr *target_sym =
          func_context_.last_float_freereg_ ? &func_context_.float_freereg1_ : &func_context_.float_freereg2_;
      {
        int other_reg =
            (!func_context_.last_float_freereg_) ? 0 : func_context_.func_attr_.attr.used_regs.floatReservedReg;
        SymbolPtr *other_sym =
            (!func_context_.last_float_freereg_) ? &func_context_.float_freereg1_ : &func_context_.float_freereg2_;
        if (target_reg == except_reg ||
            (other_reg != except_reg && ((*other_sym) == nullptr) || (*other_sym)->IsLiteral())) {
          target_reg = other_reg;
          target_sym = other_sym;
        }
      }
      if (*target_sym != nullptr) {
        evit_float_reg(target_reg);
      }
      if (sym->IsLiteral()) {
        int freeintreg = get_free_int_reg();
        uint32_t castval = ArmHelper::BitcastToUInt(sym->value_.GetFloat());
        if (ArmHelper::IsImmediateValue(castval)) {
          emitln("mov " + IntRegIDToName(freeintreg) + ", #" + std::to_string(castval));
        } else {
          emitln("ldr " + IntRegIDToName(freeintreg) + ", =" + std::to_string(castval));
        }
        emitln("vmov s" + std::to_string(target_reg) + ", " + IntRegIDToName(freeintreg));

      } else {
        auto attr = func_context_.reg_alloc_->get_SymAttribute(sym);
        int32_t realoffset = getrealoffset(attr);
        if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
          emitln("vldr s" + std::to_string(target_reg) + ", [sp, #" + std::to_string(realoffset) + "]");
        } else {
          int freeintreg = get_free_int_reg();
          emitln("ldr " + IntRegIDToName(freeintreg) + ", =" + std::to_string(realoffset));
          emitln("add " + IntRegIDToName(freeintreg) + ", " + IntRegIDToName(freeintreg) + ", sp");
          emitln("vldr s" + std::to_string(target_reg) + ", [" + IntRegIDToName(freeintreg) + "]");
        }
      }

      //最近使用的是target_reg
      func_context_.last_float_freereg_ = target_reg;
      return target_reg;
    } else {
      //尽量选择一个既不是经常被用到，又不是except_reg的reg
      int target_reg = func_context_.last_int_freereg_ ? 0 : func_context_.func_attr_.attr.used_regs.intReservedReg;
      SymbolPtr *target_sym =
          func_context_.last_int_freereg_ ? &func_context_.int_freereg1_ : &func_context_.int_freereg2_;
      {
        int other_reg = (!func_context_.last_int_freereg_) ? 0 : func_context_.func_attr_.attr.used_regs.intReservedReg;
        SymbolPtr *other_sym =
            (!func_context_.last_int_freereg_) ? &func_context_.int_freereg1_ : &func_context_.int_freereg2_;
        if (target_reg == except_reg || (other_reg != except_reg && (*other_sym) == nullptr)) {
          target_reg = other_reg;
          target_sym = other_sym;
        }
      }
      if (*target_sym != nullptr) {
        evit_int_reg(target_reg);
      }
      if (sym->IsLiteral()) {
        int val = sym->value_.GetInt();
        if (ArmHelper::IsImmediateValue(val)) {
          emitln("mov " + IntRegIDToName(target_reg) + ", #" + std::to_string(val));
        } else {
          emitln("ldr " + IntRegIDToName(target_reg) + ", =" + std::to_string(val));
        }
      } else {
        auto attr = func_context_.reg_alloc_->get_SymAttribute(sym);
        int32_t realoffset = getrealoffset(attr);
        if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
          emitln("ldr " + IntRegIDToName(target_reg) + ", [sp, #" + std::to_string(realoffset) + "]");
        } else {
          emitln("ldr " + IntRegIDToName(target_reg) + ", =" + std::to_string(realoffset));
          emitln("ldr " + IntRegIDToName(target_reg) + ", [sp, " + IntRegIDToName(target_reg) + "]");
        }
      }
      //最近使用的是target_reg
      func_context_.last_int_freereg_ = target_reg;
      return target_reg;
    }
  };

  auto binary_operation = [&, this]() -> void {

  };

  auto unary_operation = [&, this]() -> void {
    if (tac->operation_ == TACOperationType::UnaryMinus) {
      //如果是转负数运算
      if (tac->a_->value_.UnderlyingType() != tac->b_->value_.UnderlyingType()) {
        throw std::logic_error("Mismatch type in unary minus");
      }
      int valreg = alloc_reg(tac->b_);
      int dstreg = alloc_reg(tac->a_, valreg);
      //如果是浮点
      if (tac->b_->value_.UnderlyingType() == SymbolValue::ValueType::Float) {
        emitln("vneg.f32 s" + std::to_string(dstreg) + ", s" + std::to_string(valreg));
      } else {
        emitln("neg " + IntRegIDToName(dstreg) + ", " + IntRegIDToName(valreg));
      }
    } else {
      //如果是判断是否为0的 ! 逻辑运算
      if (tac->a_->value_.UnderlyingType() != SymbolValue::ValueType::Int) {
        throw std::logic_error("Dest symbol must be int in unary not");
      }
      int valreg = alloc_reg(tac->b_);
      int dstreg = alloc_reg(tac->a_, valreg);
      if (tac->b_->value_.UnderlyingType() == SymbolValue::ValueType::Float) {
        emitln("vcmp.f32 s" + std::to_string(valreg) + ", #0");
      } else {
        emitln("cmp " + std::to_string(valreg) + ", #0");
      }
      emitln("moveq " + IntRegIDToName(dstreg) + ", #1");
      emitln("movne " + IntRegIDToName(dstreg) + ", #0");
    }
  };
  
  auto assignment = [&, this]() -> void {
    if (tac->b_ == tac->a_) {
      //无需赋值
      return;
    }
    int valreg = alloc_reg(tac->b_);
    int dstreg = alloc_reg(tac->a_, valreg);
    bool arrayA = tac->a_->value_.Type() == SymbolValue::ValueType::Array;
    bool arrayB = tac->b_->value_.Type() == SymbolValue::ValueType::Array;
    if(arrayA && arrayB){
      //不符合语法
      throw std::logic_error("Cant assign array element to array element");
    }
    if (arrayA) {
      //TODO: 
    }
    if(arrayB){
      //TODO: 
    }

    if (tac->b_->value_.UnderlyingType() == SymbolValue::ValueType::Float) {
      emitln("vmov.f32 s" + std::to_string(dstreg) + ", s" + std::to_string(valreg));
    } else {
      emitln("mov " + IntRegIDToName(dstreg) + ", " + IntRegIDToName(valreg));
    }
  };
  
  auto branch = [&, this]() -> void {
    //TODO: 保存寄存器
    if (tac->operation_ == TACOperationType::Goto) {
      if(tac->a_->type_ != SymbolType::Label){
        throw std::logic_error("Must goto a label");
      }
      emitln("b " + tac->a_->get_tac_name(true));
    }else{
      //是IfZero
      // a是label b是cond
      int valreg = alloc_reg(tac->b_);
      if (tac->b_->value_.UnderlyingType() == SymbolValue::ValueType::Float) {
        emitln("vcmp.f32 s" + std::to_string(valreg) + ", #0");
      } else {
        emitln("cmp " + IntRegIDToName(valreg) + ", #0");
      }
      emitln("beq " + tac->a_->get_tac_name(true));
    }
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
        SET_UINT(func_context_.intregs_, symAttr.value);
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
        SET_UINT(func_context_.floatregs_, symAttr.value);
        if (!on_stack) {
          if (offset == 0) {
            if (symAttr.value != 0) {
              emitln("vmov.f32 s" + std::to_string(symAttr.value) + ", s0");
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

  auto label_declaration = [&, this]() -> void { emitln(tac->a_->get_tac_name(true) + ":"); };

  auto array_declaration = [&, this]() -> void {
    auto arrayAttr = func_context_.reg_alloc_->get_ArrayAttribute(tac->a_);
    int reg = alloc_reg(tac->a_);
    int32_t realoffset = arrayAttr.value + func_context_.stack_size_for_args_;
    if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
      emitln("add " + IntRegIDToName(reg) + ", sp, #" + std::to_string(realoffset));
    } else {
      emitln("ldr " + IntRegIDToName(reg) + ", =" + std::to_string(realoffset));
      emitln("add " + IntRegIDToName(reg) + ", sp, " + IntRegIDToName(reg));
    }
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
    case TACOperationType::UnaryNot:
      func_context_.parameter_head_ = false;
      unary_operation();
      break;
    case TACOperationType::UnaryPositive:
    // 单元正 其实和 Assign 等价
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

    case TACOperationType::Variable:
    case TACOperationType::Constant: {
      func_context_.parameter_head_ = false;
      if (tac->a_->value_.Type() == SymbolValue::ValueType::Array) {
        array_declaration();
      }
      break;
    }
    case TACOperationType::Label: {
      func_context_.parameter_head_ = false;
      label_declaration();
      break;
    }
    case TACOperationType::BlockBegin:
    case TACOperationType::BlockEnd: {
      func_context_.parameter_head_ = false;
      // ignore
      break;
    }
    default:
      throw std::runtime_error("Unexpected operation: " +
                               std::string(magic_enum::enum_name<TACOperationType>(tac->operation_)));
      break;
  }

  return ret;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler