#include <algorithm>
#include <iostream>
#include <utility>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/arm/ArmBuilder.hh"
#include "ASM/arm/ArmHelper.hh"
#include "ASM/arm/FunctionContext.hh"
#include "ASM/arm/RegAllocator.hh"
#include "MagicEnum.hh"
#include "TAC/TAC.hh"
#include "Utility.hh"

namespace HaveFunCompiler {
using namespace ThreeAddressCode;
namespace AssemblyBuilder {

std::string ArmBuilder::FuncTACToASMString(TACPtr tac) {
  std::string ret = "";
  auto emitln = [&ret, this](const std::string &inst) -> void {
    ret.append(inst);
    ret.append("\n");
    data_pool_distance_ += ArmHelper::CountLines(inst) + 1;
    if (data_pool_distance_ > DATA_POOL_DISTANCE_THRESHOLD) {
      ret += EndCurrentDataPool();
    }
  };
  //来个注释好了
  emitln("// " + tac->ToString());

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

  // reg_id为0时驱逐int_freereg1，否则驱逐int_freereg2。 只会用到reg_id一个寄存器。
  auto evit_int_reg = [&, this](int reg_id) -> void {
    SymbolPtr *target_sym;
    if (reg_id) {
      target_sym = &func_context_.int_freereg2_;
      reg_id = func_context_.func_attr_.attr.used_regs.intReservedReg;
    } else {
      target_sym = &func_context_.int_freereg1_;
    }
    if (*target_sym == nullptr) {
      //不需要驱逐
      return;
    }
    if ((*target_sym)->IsLiteral()) {
      //字面量不用回存。
      *target_sym = nullptr;
      return;
    }
    if ((*target_sym)->IsGlobal()) {
      //数组指针不会被更改，直接就不用会存。
      if ((*target_sym)->value_.Type() != SymbolValue::ValueType::Array) {
        int otherreg = reg_id ? 0 : func_context_.func_attr_.attr.used_regs.intReservedReg;
        emitln("push {" + IntRegIDToName(otherreg) + "}");
        emitln("ldr " + IntRegIDToName(otherreg) + ", " + ToDataRefName(GetVariableName(*target_sym)));
        emitln("str " + IntRegIDToName(reg_id) + ", [" + IntRegIDToName(otherreg) + "]");
        emitln("pop {" + IntRegIDToName(otherreg) + "}");
      }
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
      //如果不能直接做，我们对sp加上相应偏移量，使得每一个偏移量都可以。
      auto splitoffset = ArmHelper::DivideIntoImmediateValues(realoffset);
      for (size_t i = 0; i + 1 < splitoffset.size(); i++) {
        emitln("add sp, sp, #" + std::to_string(splitoffset[i]));
      }
      //如果最后一个偏移量能直接用立即数放str里面，就直接用了。
      bool canbeimm = ArmHelper::IsLDRSTRImmediateValue(splitoffset.back());
      if (canbeimm) {
        emitln("str " + IntRegIDToName(reg_id) + ", [sp, #" + std::to_string(splitoffset.back()) + "]");
        splitoffset.pop_back();
      } else {
        emitln("add sp, sp, #" + std::to_string(splitoffset.back()));
        emitln("str " + IntRegIDToName(reg_id) + ", [sp]");
      }
      //把sp减回去
      for (auto it = splitoffset.rbegin(); it != splitoffset.rend(); it++) {
        bool check = ArmHelper::EmitImmediateInstWithCheck(emitln, "sub", "sp", "sp", *it);
        assert(check);
      }
    }
    *target_sym = nullptr;
  };

  // 自动选择驱逐一个不常用的freereg，并返回其编号
  auto get_free_int_reg = [&, this]() -> int {
    bool empty0 = func_context_.int_freereg1_ == nullptr || func_context_.int_freereg1_->IsLiteral();
    bool empty1 = func_context_.int_freereg2_ == nullptr || func_context_.int_freereg2_->IsLiteral();
    if (empty0 && empty1) {
      if (func_context_.last_int_freereg_ == 0) {
        return func_context_.func_attr_.attr.used_regs.intReservedReg;
      } else {
        return 0;
      }
    }
    if (empty0) {
      return 0;
    }
    if (empty1) {
      return func_context_.func_attr_.attr.used_regs.intReservedReg;
    }
    //如果上一个在用int_freereg1，则驱逐reg2
    if (func_context_.last_int_freereg_ == 0) {
      int reg_id = func_context_.func_attr_.attr.used_regs.intReservedReg;
      evit_int_reg(reg_id);
      return reg_id;
    } else {
      evit_int_reg(0);
      return 0;
    }
  };

  // reg_id为0时驱逐float_freereg1，否则驱逐float_freereg2。只会用到reg_id一个寄存器以及可能一个通用寄存器。
  auto evit_float_reg = [&, this](int reg_id) -> void {
    SymbolPtr *target_sym;
    if (reg_id) {
      target_sym = &func_context_.float_freereg2_;
      reg_id = func_context_.func_attr_.attr.used_regs.floatReservedReg;
    } else {
      target_sym = &func_context_.float_freereg1_;
    }
    if (*target_sym == nullptr) {
      //不需要驱逐
      return;
    }
    if ((*target_sym)->IsLiteral()) {
      //字面量不用回存。
      *target_sym = nullptr;
      return;
    }
    if ((*target_sym)->IsGlobal()) {
      int freeintreg = get_free_int_reg();
      emitln("ldr " + IntRegIDToName(freeintreg) + ", " + ToDataRefName(GetVariableName(*target_sym)));
      emitln("vstr " + FloatRegIDToName(reg_id) + ", [" + IntRegIDToName(freeintreg) + "]");
      *target_sym = nullptr;
      return;
    }
    auto attr = func_context_.reg_alloc_->get_SymAttribute(*target_sym);
    if (attr.attr.store_type == attr.INT_REG || attr.attr.store_type == attr.FLOAT_REG) {
      throw std::logic_error("Cannot evit register variable");
    }

    int32_t realoffset = getrealoffset(attr);

    if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
      emitln("vstr " + FloatRegIDToName(reg_id) + ", [sp, #" + std::to_string(realoffset) + "]");
    } else {
      //如果不能直接做，我们对sp加上相应偏移量，使得每一个偏移量都可以。
      auto splitoffset = ArmHelper::DivideIntoImmediateValues(realoffset);
      for (size_t i = 0; i + 1 < splitoffset.size(); i++) {
        emitln("add sp, sp, #" + std::to_string(splitoffset[i]));
      }
      //如果最后一个偏移量能直接用立即数放str里面，就直接用了。
      bool canbeimm = ArmHelper::IsLDRSTRImmediateValue(splitoffset.back());
      if (canbeimm) {
        emitln("vstr " + FloatRegIDToName(reg_id) + ", [sp, #" + std::to_string(splitoffset.back()) + "]");
        splitoffset.pop_back();
      } else {
        emitln("add sp, sp, #" + std::to_string(splitoffset.back()));
        emitln("vstr " + FloatRegIDToName(reg_id) + ", [sp]");
      }
      //把sp减回去
      for (auto it = splitoffset.rbegin(); it != splitoffset.rend(); it++) {
        bool check = ArmHelper::EmitImmediateInstWithCheck(emitln, "sub", "sp", "sp", *it);
        assert(check);
      }
    }
    *target_sym = nullptr;
  };

  auto evit_all_freereg = [&, this]() -> void {
    evit_int_reg(0);
    evit_int_reg(1);
    evit_float_reg(0);
    evit_float_reg(1);
  };

  // 自动选择驱逐一个不常用的freereg，并返回其编号
  auto get_free_float_reg = [&, this]() -> int {
    bool empty0 = func_context_.float_freereg1_ == nullptr || func_context_.float_freereg1_->IsLiteral();
    bool empty1 = func_context_.float_freereg2_ == nullptr || func_context_.float_freereg2_->IsLiteral();
    if (empty0 && empty1) {
      if (func_context_.last_float_freereg_ == 0) {
        return func_context_.func_attr_.attr.used_regs.floatReservedReg;
      } else {
        return 0;
      }
    }
    if (empty0) {
      return 0;
    }
    if (empty1) {
      return func_context_.func_attr_.attr.used_regs.floatReservedReg;
    }
    //如果上一个在用float_freereg1，则驱逐reg2
    if (func_context_.last_float_freereg_ == 0) {
      int reg_id = func_context_.func_attr_.attr.used_regs.floatReservedReg;
      evit_float_reg(reg_id);
      return reg_id;
    } else {
      evit_float_reg(0);
      return 0;
    }
  };

  //分配除了except_reg之外的一个reg。但若reg_alloc有指示，或者已经在寄存器中缓存，则无视except_reg
  auto alloc_reg = [&, this](SymbolPtr sym, int except_reg = -1, bool no_load = false) -> int {
    //如果有缓存就直接用
    bool is_float = (sym->value_.Type() == SymbolValue::ValueType::Float);
    if (sym->value_.Type() == SymbolValue::ValueType::Array) {
      sym = sym->value_.GetArrayDescriptor()->base_addr.lock();
    }
    if (is_float) {
      if (func_context_.float_freereg1_ == sym) {
        func_context_.last_float_freereg_ = 0;
        return 0;
      }
      if (func_context_.float_freereg2_ == sym) {
        func_context_.last_float_freereg_ = func_context_.func_attr_.attr.used_regs.floatReservedReg;
        return func_context_.func_attr_.attr.used_regs.floatReservedReg;
      }
    } else {
      if (func_context_.int_freereg1_ == sym) {
        func_context_.last_int_freereg_ = 0;
        return 0;
      }
      if (func_context_.int_freereg2_ == sym) {
        func_context_.last_int_freereg_ = func_context_.func_attr_.attr.used_regs.intReservedReg;
        return func_context_.func_attr_.attr.used_regs.intReservedReg;
      }
    }

    if (!sym->IsLiteral() && !sym->IsGlobal()) {
      auto attr = func_context_.reg_alloc_->get_SymAttribute(sym);
      if (attr.attr.store_type == attr.FLOAT_REG || attr.attr.store_type == attr.INT_REG) {
        // reg_alloc已经有指示了，直接用就好了。
        return attr.value;
      }
    }

    //如果没缓存
    if (is_float) {
      //尽量选择一个既不是经常被用到，又不是except_reg的reg
      int target_reg;
      SymbolPtr *target_sym;
      {
        int regids[2] = {0, func_context_.func_attr_.attr.used_regs.floatReservedReg};
        SymbolPtr *syms[2] = {&func_context_.float_freereg1_, &func_context_.float_freereg2_};
        auto choose = [&](int id) -> void {
          target_reg = regids[id];
          target_sym = syms[id];
        };

        if (except_reg == regids[0] || except_reg == regids[1]) {
          if (except_reg == regids[0]) {
            choose(1);
          } else {
            choose(0);
          }
        } else {
          bool empty0 = (*syms[0]) == nullptr || (*syms[0])->IsLiteral();
          bool empty1 = (*syms[1]) == nullptr || (*syms[1])->IsLiteral();
          if ((empty0 && empty1) || ((!empty0) && (!empty1))) {
            if (func_context_.last_float_freereg_ == 0) {
              choose(1);
            } else {
              choose(0);
            }
          } else {
            if (empty0) {
              choose(0);
            } else {
              choose(1);
            }
          }
        }
      }

      evit_float_reg(target_reg);
      *target_sym = sym;
      if (!no_load) {
        if (sym->IsLiteral()) {
          int freeintreg = get_free_int_reg();
          uint32_t castval = ArmHelper::BitcastToUInt(sym->value_.GetFloat());
          if (ArmHelper::IsImmediateValue(castval)) {
            emitln("mov " + IntRegIDToName(freeintreg) + ", #" + std::to_string(castval));
          } else {
            emitln("ldr " + IntRegIDToName(freeintreg) + ", =" + std::to_string(castval));
          }
          emitln("vmov " + FloatRegIDToName(target_reg) + ", " + IntRegIDToName(freeintreg));
        } else if (sym->IsGlobal()) {
          int freeintreg = get_free_int_reg();
          emitln("ldr " + IntRegIDToName(freeintreg) + ", " + ToDataRefName(GetVariableName(sym)));
          emitln("vldr " + FloatRegIDToName(target_reg) + ", [" + IntRegIDToName(freeintreg) + "]");
        } else {
          auto attr = func_context_.reg_alloc_->get_SymAttribute(sym);
          int32_t realoffset = getrealoffset(attr);
          if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
            emitln("vldr " + FloatRegIDToName(target_reg) + ", [sp, #" + std::to_string(realoffset) + "]");
          } else {
            int freeintreg = get_free_int_reg();
            emitln("ldr " + IntRegIDToName(freeintreg) + ", =" + std::to_string(realoffset));
            // emitln("add " + IntRegIDToName(freeintreg) + ", " + IntRegIDToName(freeintreg) + ", sp");
            emitln("vldr " + FloatRegIDToName(target_reg) + ", [sp, " + IntRegIDToName(freeintreg) + "]");
          }
        }
      }

      //最近使用的是target_reg
      func_context_.last_float_freereg_ = target_reg;
      return target_reg;
    } else {
      //尽量选择一个既不是经常被用到，又不是except_reg的reg
      int target_reg;
      SymbolPtr *target_sym;
      {
        int regids[2] = {0, func_context_.func_attr_.attr.used_regs.intReservedReg};
        SymbolPtr *syms[2] = {&func_context_.int_freereg1_, &func_context_.int_freereg2_};
        auto choose = [&](int id) -> void {
          target_reg = regids[id];
          target_sym = syms[id];
        };

        if (except_reg == regids[0] || except_reg == regids[1]) {
          if (except_reg == regids[0]) {
            choose(1);
          } else {
            choose(0);
          }
        } else {
          bool empty0 = (*syms[0]) == nullptr || (*syms[0])->IsLiteral();
          bool empty1 = (*syms[1]) == nullptr || (*syms[1])->IsLiteral();
          if ((empty0 && empty1) || ((!empty0) && (!empty1))) {
            if (func_context_.last_int_freereg_ == 0) {
              choose(1);
            } else {
              choose(0);
            }
          } else {
            if (empty0) {
              choose(0);
            } else {
              choose(1);
            }
          }
        }
      }

      evit_int_reg(target_reg);
      *target_sym = sym;

      if (!no_load) {
        if (sym->IsLiteral()) {
          int val = sym->value_.GetInt();
          if (ArmHelper::IsImmediateValue(val)) {
            emitln("mov " + IntRegIDToName(target_reg) + ", #" + std::to_string(val));
          } else {
            emitln("ldr " + IntRegIDToName(target_reg) + ", =" + std::to_string(val));
          }
        } else if (sym->IsGlobal()) {
          emitln("ldr " + IntRegIDToName(target_reg) + ", " + ToDataRefName(GetVariableName(sym)));
          if (sym->value_.Type() != SymbolValue::ValueType::Array) {
            emitln("ldr " + IntRegIDToName(target_reg) + ", [" + IntRegIDToName(target_reg) + "]");
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
      }

      //最近使用的是target_reg
      func_context_.last_int_freereg_ = target_reg;
      return target_reg;
    }
  };

  //查看此符号是否被分配永久寄存器
  auto symbol_reg = [&, this](SymbolPtr sym) -> int {
    if (sym->IsLiteral() || sym->IsGlobal()) {
      return -1;
    }
    auto attr = func_context_.reg_alloc_->get_SymAttribute(sym);
    //如果被分配的话返回寄存器编号
    if (attr.attr.store_type == attr.FLOAT_REG || attr.attr.store_type == attr.INT_REG) {
      return attr.value;
    }
    //否则返回-1
    return -1;
  };

  //处理三个都是同种寄存器时的情况。返回值为：res寄存器，op1寄存器，op2寄存器，是否需要在运算后将缓存去除(-1,0,1，-1代表不需要，0表示去除第一个自由寄存器，1代表去除第二个)。
  auto prepare_binary_operation = [&, this](SymbolPtr result, SymbolPtr operand1,
                                            SymbolPtr operand2) -> std::tuple<int, int, int, int> {
    assert(result->value_.Type() == operand1->value_.Type());
    assert(operand1->value_.Type() == operand2->value_.Type());
    // assert(operand1 != operand2);
    int resreg = -1;
    int op1reg = alloc_reg(operand1);
    int op2reg = alloc_reg(operand2, op1reg);
    int freeregid = -1;
    bool inres = false;
    if (result == operand1) {
      resreg = op1reg;
      inres = true;
    } else if (result == operand2) {
      resreg = op2reg;
      inres = true;
    }
    //当result不是两个操作数之一时
    //如果op1不是寄存器变量
    // resreg复用op1reg，需要将freeregid正确标记
    //否则，op1是寄存器变量，给resreg另请高明，但是不需要freeregid了（因为没有复用）
    if (!inres) {
      resreg = symbol_reg(result);
      if (resreg == -1) {
        //如果res被分配在栈上
        if (result->value_.Type() == SymbolValue::ValueType::Float) {
          if (symbol_reg(operand1) == -1) {
            evit_float_reg(op1reg);
            resreg = op1reg;
            freeregid = (op1reg == func_context_.func_attr_.attr.used_regs.floatReservedReg);
          } else {
            resreg = alloc_reg(result, op2reg, true);
            freeregid = -1;
          }
        } else {
          if (symbol_reg(operand1) == -1) {
            evit_int_reg(op1reg);
            resreg = op1reg;
            freeregid = (op1reg == func_context_.func_attr_.attr.used_regs.intReservedReg);
          } else {
            resreg = alloc_reg(result, op2reg, true);
            freeregid = -1;
          }
        }
      }
      //如果res被分配在寄存器，可以直接用，也不需要freeregid
    }
    return {resreg, op1reg, op2reg, freeregid};
  };

  // mod很有可能有问题
  auto binary_operation = [&, this]() -> void {
    assert(tac->a_->value_.Type() == tac->b_->value_.Type());
    assert(tac->b_->value_.Type() == tac->c_->value_.Type());
    std::tuple<int, int, int, int> prepare_res;
    if (!(tac->a_->value_.Type() != SymbolValue::ValueType::Float &&
          (tac->operation_ == TACOperationType::Div || tac->operation_ == TACOperationType::Mod) &&
          tac->c_->type_ == SymbolType::Constant && ArmHelper::IsPowerOf2(tac->c_->value_.GetInt()))) {
      //对于不可以优化的Div和Mod之外的操作才进行prepare
      prepare_res = prepare_binary_operation(tac->a_, tac->b_, tac->c_);
    }

    auto [resreg, op1reg, op2reg, freeregid] = prepare_res;

    if (tac->a_->value_.Type() == SymbolValue::ValueType::Float) {
      switch (tac->operation_) {
        case TACOperationType::Add: {
          emitln("vadd.f32 " + FloatRegIDToName(resreg) + ", " + FloatRegIDToName(op1reg) + ", " +
                 FloatRegIDToName(op2reg));
          break;
        }
        case TACOperationType::Div: {
          emitln("vdiv.f32 " + FloatRegIDToName(resreg) + ", " + FloatRegIDToName(op1reg) + ", " +
                 FloatRegIDToName(op2reg));
          break;
        }
        case TACOperationType::LessOrEqual:
        case TACOperationType::LessThan:
        case TACOperationType::NotEqual:
        case TACOperationType::GreaterOrEqual:
        case TACOperationType::GreaterThan:
        case TACOperationType::Equal: {
          emitln("vcmp.f32 " + FloatRegIDToName(op1reg) + ", " + FloatRegIDToName(op2reg));
          emitln("vmrs APSR_nzcv, FPSCR");
          int freeintreg = get_free_int_reg();
          switch (tac->operation_) {
            case TACOperationType::LessOrEqual:
              emitln("movle " + IntRegIDToName(freeintreg) + ", #1065353216");
              emitln("movgt " + IntRegIDToName(freeintreg) + ", #0");
              break;
            case TACOperationType::LessThan:
              emitln("movlt " + IntRegIDToName(freeintreg) + ", #1065353216");
              emitln("movge " + IntRegIDToName(freeintreg) + ", #0");
              break;
            case TACOperationType::NotEqual:
              emitln("movne " + IntRegIDToName(freeintreg) + ", #1065353216");
              emitln("moveq " + IntRegIDToName(freeintreg) + ", #0");
              break;
            case TACOperationType::GreaterOrEqual:
              emitln("movge " + IntRegIDToName(freeintreg) + ", #1065353216");
              emitln("movlt " + IntRegIDToName(freeintreg) + ", #0");
              break;
            case TACOperationType::GreaterThan:
              emitln("movgt " + IntRegIDToName(freeintreg) + ", #1065353216");
              emitln("movle " + IntRegIDToName(freeintreg) + ", #0");
              break;
            case TACOperationType::Equal:
              emitln("moveq " + IntRegIDToName(freeintreg) + ", #1065353216");
              emitln("movne " + IntRegIDToName(freeintreg) + ", #0");
              break;
            default:
              throw std::logic_error("Unreachable");
          }
          emitln("vmov " + FloatRegIDToName(resreg) + ", " + IntRegIDToName(freeintreg));
          break;
        }
        case TACOperationType::Mod: {
          emitln("vdiv.f32 " + FloatRegIDToName(resreg) + ", " + FloatRegIDToName(op1reg) + ", " +
                 FloatRegIDToName(op2reg));
          emitln("vmul.f32 " + FloatRegIDToName(resreg) + ", " + FloatRegIDToName(resreg) + ", " +
                 FloatRegIDToName(op2reg));
          if (freeregid != -1) {
            if (freeregid == 0) {
              // assert(func_context_.float_freereg1_ == tac->b_);
              func_context_.float_freereg1_ = nullptr;
            } else {
              // assert(func_context_.float_freereg2_ == tac->b_);
              func_context_.float_freereg2_ = nullptr;
            }
            op1reg = alloc_reg(tac->b_, resreg);
          }
          emitln("vsub.f32 " + FloatRegIDToName(resreg) + ", " + FloatRegIDToName(op1reg) + ", " +
                 FloatRegIDToName(resreg));
          if (freeregid != -1) {
            if (freeregid == 0) {
              func_context_.float_freereg1_ = tac->a_;
            } else {
              func_context_.float_freereg2_ = tac->a_;
            }
          }
          break;
        }
        case TACOperationType::Mul: {
          emitln("vmul.f32 " + FloatRegIDToName(resreg) + ", " + FloatRegIDToName(op1reg) + ", " +
                 FloatRegIDToName(op2reg));
          break;
        }
        case TACOperationType::Sub: {
          emitln("vsub.f32 " + FloatRegIDToName(resreg) + ", " + FloatRegIDToName(op1reg) + ", " +
                 FloatRegIDToName(op2reg));
          break;
        }

        default:
          throw std::logic_error("Unknown binary operation " +
                                 std::string(magic_enum::enum_name<TACOperationType>(tac->operation_)));
      }
      if (freeregid != -1 && tac->operation_ != TACOperationType::Mod) {
        emitln("vmov.f32 " + FloatRegIDToName(alloc_reg(tac->a_, resreg)) + ", " + FloatRegIDToName(resreg));
        if (freeregid == 0) {
          // assert(func_context_.float_freereg1_ == tac->b_);
          func_context_.float_freereg1_ = nullptr;
        } else {
          // assert(func_context_.float_freereg2_ == tac->b_);
          func_context_.float_freereg2_ = nullptr;
        }
      }
    } else {
      switch (tac->operation_) {
        case TACOperationType::Add: {
          emitln("add " + IntRegIDToName(resreg) + ", " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(op2reg));
          break;
        }
        case TACOperationType::Div: {
          if (tac->c_->type_ == SymbolType::Constant && ArmHelper::IsPowerOf2(tac->c_->value_.GetInt())) {
            int imm = tac->c_->value_.GetInt();
            int p = ArmHelper::Log2(imm);
            int mask = imm - 1;

            int resreg = alloc_reg(tac->a_, -1, true);
            int op1reg = alloc_reg(tac->b_, resreg);
            if (op1reg != 0 && op1reg != func_context_.func_attr_.attr.used_regs.intReservedReg) {
              if (resreg == 0 || resreg == func_context_.func_attr_.attr.used_regs.intReservedReg) {
                func_context_.last_int_freereg_ = resreg;
                evit_int_reg(!resreg);
              }
              int otherreg = get_free_int_reg();
              emitln("mov " + IntRegIDToName(otherreg) + ", " + IntRegIDToName(op1reg));
              op1reg = otherreg;
            }
            if (!ArmHelper::EmitImmediateInstWithCheck(emitln, "add", IntRegIDToName(resreg), IntRegIDToName(op1reg),
                                                       mask)) {
              emitln("ldr " + IntRegIDToName(resreg) + ", =" + std::to_string(mask));
              emitln("add " + IntRegIDToName(resreg) + ", " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(resreg));
            }
            emitln("cmp " + IntRegIDToName(op1reg) + ", #0");
            emitln("movlt " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(resreg));
            // emitln("movge " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(op1reg));
            emitln("asr " + IntRegIDToName(resreg) + ", " + IntRegIDToName(op1reg) + ", #" + std::to_string(p));
            if (op1reg == 0 || op1reg == func_context_.func_attr_.attr.used_regs.intReservedReg) {
              if (op1reg == 0) {
                func_context_.int_freereg1_ = nullptr;
              } else {
                func_context_.int_freereg2_ = nullptr;
              }
            }
          } else {
            evit_int_reg(0);
            evit_int_reg(1);
            emitln("push {r1-r3, ip}");
            if (op1reg < op2reg) {
              emitln("push {" + IntRegIDToName(op1reg) + ", " + IntRegIDToName(op2reg) + "}");
            } else {
              emitln("push {" + IntRegIDToName(op2reg) + "}");
              emitln("push {" + IntRegIDToName(op1reg) + "}");
            }
            emitln("pop {r0, r1}");
            bool padding = false;
            {
              size_t stacksize = 4 * 4ULL + func_context_.stack_size_for_args_ + func_context_.stack_size_for_regsave_ +
                                 func_context_.stack_size_for_vars_;
              if (stacksize % 8 != 0) {
                padding = true;
                emitln("sub sp, sp, #4");
              }
            }
            emitln("bl __aeabi_idiv");
            if (padding) {
              emitln("add sp, sp, #4");
            }
            emitln("pop {r1-r3, ip}");
            int regid = symbol_reg(tac->a_);
            if (regid == -1) {
              func_context_.int_freereg1_ = tac->a_;
            } else {
              emitln("mov " + IntRegIDToName(regid) + ", r0");
            }
          }

          break;
        }
        case TACOperationType::LessOrEqual:
        case TACOperationType::LessThan:
        case TACOperationType::NotEqual:
        case TACOperationType::GreaterOrEqual:
        case TACOperationType::GreaterThan:
        case TACOperationType::Equal: {
          emitln("cmp " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(op2reg));
          switch (tac->operation_) {
            case TACOperationType::LessOrEqual:
              emitln("movle " + IntRegIDToName(resreg) + ", #1");
              emitln("movgt " + IntRegIDToName(resreg) + ", #0");
              break;
            case TACOperationType::LessThan:
              emitln("movlt " + IntRegIDToName(resreg) + ", #1");
              emitln("movge " + IntRegIDToName(resreg) + ", #0");
              break;
            case TACOperationType::NotEqual:
              emitln("movne " + IntRegIDToName(resreg) + ", #1");
              emitln("moveq " + IntRegIDToName(resreg) + ", #0");
              break;
            case TACOperationType::GreaterOrEqual:
              emitln("movge " + IntRegIDToName(resreg) + ", #1");
              emitln("movlt " + IntRegIDToName(resreg) + ", #0");
              break;
            case TACOperationType::GreaterThan:
              emitln("movgt " + IntRegIDToName(resreg) + ", #1");
              emitln("movle " + IntRegIDToName(resreg) + ", #0");
              break;
            case TACOperationType::Equal:
              emitln("moveq " + IntRegIDToName(resreg) + ", #1");
              emitln("movne " + IntRegIDToName(resreg) + ", #0");
              break;
            default:
              throw std::logic_error("Unreachable");
          }
          break;
        }
        case TACOperationType::Mod: {
          if (tac->c_->type_ == SymbolType::Constant && ArmHelper::IsPowerOf2(tac->c_->value_.GetInt())) {
            int imm = tac->c_->value_.GetInt();
            int mask = imm - 1;

            int resreg = alloc_reg(tac->a_, -1, true);
            int op1reg = alloc_reg(tac->b_, resreg);

            if (op1reg != 0 && op1reg != func_context_.func_attr_.attr.used_regs.intReservedReg) {
              if (resreg == 0 || resreg == func_context_.func_attr_.attr.used_regs.intReservedReg) {
                func_context_.last_int_freereg_ = resreg;
                evit_int_reg(!resreg);
              }
              int otherreg = get_free_int_reg();
              emitln("mov " + IntRegIDToName(otherreg) + ", " + IntRegIDToName(op1reg));
              op1reg = otherreg;
            }

            emitln("rsbs " + IntRegIDToName(resreg) + ", " + IntRegIDToName(op1reg) + ", #0");
            if (ArmHelper::IsImmediateValue(mask)) {
              emitln("and " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(op1reg) + ", #" + std::to_string(mask));
              emitln("and " + IntRegIDToName(resreg) + ", " + IntRegIDToName(resreg) + ", #" + std::to_string(mask));
            } else {
              uint32_t invmask = ~((uint32_t)mask);
              auto immvals = ArmHelper::DivideIntoImmediateValues(invmask);
              for (auto imm_mask : immvals) {
                emitln("bic " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(op1reg) + ", #" +
                       std::to_string(imm_mask));
              }
              for (auto imm_mask : immvals) {
                emitln("bic " + IntRegIDToName(resreg) + ", " + IntRegIDToName(resreg) + ", #" +
                       std::to_string(imm_mask));
              }
            }
            emitln("rsbpl " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(resreg) + ", #0");
            emitln("mov " + IntRegIDToName(resreg) + ", " + IntRegIDToName(op1reg));
            if (op1reg == 0 || op1reg == func_context_.func_attr_.attr.used_regs.intReservedReg) {
              if (op1reg == 0) {
                func_context_.int_freereg1_ = nullptr;
              } else {
                func_context_.int_freereg2_ = nullptr;
              }
            }
          } else {
            evit_int_reg(0);
            evit_int_reg(1);
            emitln("push {r1-r3, ip}");
            if (op1reg < op2reg) {
              emitln("push {" + IntRegIDToName(op1reg) + ", " + IntRegIDToName(op2reg) + "}");
            } else {
              emitln("push {" + IntRegIDToName(op2reg) + "}");
              emitln("push {" + IntRegIDToName(op1reg) + "}");
            }
            emitln("pop {r0, r1}");
            bool padding = false;
            {
              size_t stacksize = 4 * 4ULL + func_context_.stack_size_for_args_ + func_context_.stack_size_for_regsave_ +
                                 func_context_.stack_size_for_vars_;
              if (stacksize % 8 != 0) {
                padding = true;
                emitln("sub sp, sp, #4");
              }
            }
            emitln("bl __aeabi_idivmod");
            if (padding) {
              emitln("add sp, sp, #4");
            }
            emitln("mov r0, r1");
            emitln("pop {r1-r3, ip}");
            int regid = symbol_reg(tac->a_);
            if (regid == -1) {
              func_context_.int_freereg1_ = tac->a_;
            } else {
              emitln("mov " + IntRegIDToName(regid) + ", r0");
            }
          }

          break;

          // emitln("sdiv " + IntRegIDToName(resreg) + ", " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(op2reg));
          // emitln("mul " + IntRegIDToName(resreg) + ", " + IntRegIDToName(resreg) + ", " + IntRegIDToName(op2reg));
          // if (freeregid != -1) {
          //   if (freeregid == 0) {
          //     assert(func_context_.int_freereg1_ == tac->b_);
          //     func_context_.int_freereg1_ = nullptr;
          //   } else {
          //     assert(func_context_.int_freereg2_ == tac->b_);
          //     func_context_.int_freereg2_ = nullptr;
          //   }
          //   op1reg = alloc_reg(tac->b_, resreg);
          // }
          // emitln("sub " + IntRegIDToName(resreg) + ", " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(resreg));
          // if (freeregid != -1) {
          //   if (freeregid == 0) {
          //     func_context_.int_freereg1_ = tac->a_;
          //   } else {
          //     func_context_.int_freereg2_ = tac->a_;
          //   }
          // }
          // break;
        }
        case TACOperationType::Mul: {
          emitln("mul " + IntRegIDToName(resreg) + ", " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(op2reg));
          break;
        }
        case TACOperationType::Sub: {
          emitln("sub " + IntRegIDToName(resreg) + ", " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(op2reg));
          break;
        }

        default:
          throw std::logic_error("Unknown binary operation " +
                                 std::string(magic_enum::enum_name<TACOperationType>(tac->operation_)));
      }
      if (freeregid != -1 && tac->operation_ != TACOperationType::Mod && tac->operation_ != TACOperationType::Div) {
        emitln("mov " + IntRegIDToName(alloc_reg(tac->a_, resreg)) + ", " + IntRegIDToName(resreg));
        if (freeregid == 0) {
          // assert(func_context_.int_freereg1_ == tac->b_);
          func_context_.int_freereg1_ = nullptr;
        } else {
          // assert(func_context_.int_freereg2_ == tac->b_);
          func_context_.int_freereg2_ = nullptr;
        }
      }
    }
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
        emitln("vneg.f32 " + FloatRegIDToName(dstreg) + ", " + FloatRegIDToName(valreg));
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
        emitln("vcmp.f32 " + FloatRegIDToName(valreg) + ", #0");
        emitln("vmrs APSR_nzcv, FPSCR");
      } else {
        emitln("cmp " + IntRegIDToName(valreg) + ", #0");
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
    bool arrayA = tac->a_->value_.Type() == SymbolValue::ValueType::Array;
    bool arrayB = tac->b_->value_.Type() == SymbolValue::ValueType::Array;
    if (arrayA && arrayB) {
      //不符合语法
      throw std::logic_error("Cant assign array element to array element");
    }
    if (arrayA) {
      auto arrayDescriptor = tac->a_->value_.GetArrayDescriptor();
      auto basesym = arrayDescriptor->base_addr.lock();
      int basereg = alloc_reg(basesym);
      int addrreg;
      {
        int offreg = alloc_reg(arrayDescriptor->base_offset, basereg);
        if (basereg == 0 || basereg == func_context_.func_attr_.attr.used_regs.intReservedReg) {
          if (basereg == 0) {
            // assert(func_context_.int_freereg1_ == basesym);
            func_context_.int_freereg1_ = nullptr;
          } else {
            // assert(func_context_.int_freereg2_ == basesym);
            func_context_.int_freereg2_ = nullptr;
          }
          addrreg = basereg;
          //把offreg的另外一个reg驱逐。
          evit_int_reg((addrreg == 0));
          func_context_.last_int_freereg_ = addrreg;
        } else {
          //把offreg的另外一个reg驱逐。
          evit_int_reg((offreg == 0));
          addrreg = offreg ? 0 : func_context_.func_attr_.attr.used_regs.intReservedReg;
          func_context_.last_int_freereg_ = addrreg;
        }
        emitln("add " + IntRegIDToName(addrreg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg) +
               ", LSL #2");
      }
      int valuereg = alloc_reg(tac->b_, addrreg);
      if (tac->b_->value_.Type() == SymbolValue::ValueType::Float) {
        emitln("vstr " + FloatRegIDToName(valuereg) + ", [" + IntRegIDToName(addrreg) + "]");
      } else {
        emitln("str " + IntRegIDToName(valuereg) + ", [" + IntRegIDToName(addrreg) + "]");
      }
    } else if (arrayB) {
      auto arrayDescriptor = tac->b_->value_.GetArrayDescriptor();
      auto basesym = arrayDescriptor->base_addr.lock();
      int basereg = alloc_reg(basesym);
      int addrreg;
      {
        int offreg = alloc_reg(arrayDescriptor->base_offset, basereg);
        if (basereg == 0 || basereg == func_context_.func_attr_.attr.used_regs.intReservedReg) {
          if (basereg == 0) {
            // assert(func_context_.int_freereg1_ == basesym);
            func_context_.int_freereg1_ = nullptr;
          } else {
            // assert(func_context_.int_freereg2_ == basesym);
            func_context_.int_freereg2_ = nullptr;
          }
          addrreg = basereg;

          //把offreg的另外一个reg驱逐。
          evit_int_reg((addrreg == 0));
          func_context_.last_int_freereg_ = addrreg;
        } else {
          //把offreg的另外一个reg驱逐。
          evit_int_reg((offreg == 0));
          addrreg = offreg ? 0 : func_context_.func_attr_.attr.used_regs.intReservedReg;
          func_context_.last_int_freereg_ = addrreg;
        }
        emitln("add " + IntRegIDToName(addrreg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg) +
               ", LSL #2");
      }
      int valuereg = alloc_reg(tac->a_, addrreg);
      if (tac->a_->value_.Type() == SymbolValue::ValueType::Float) {
        emitln("vldr " + FloatRegIDToName(valuereg) + ", [" + IntRegIDToName(addrreg) + "]");
      } else {
        emitln("ldr " + IntRegIDToName(valuereg) + ", [" + IntRegIDToName(addrreg) + "]");
      }
    } else if (tac->a_->IsGlobal()) {
      int dstreg = get_free_int_reg();
      int valreg;
      emitln("ldr " + IntRegIDToName(dstreg) + ", " + ToDataRefName(GetVariableName(tac->a_)));
      if (tac->a_->value_.Type() == SymbolValue::ValueType::Float) {
        valreg = alloc_reg(tac->b_);
        if (func_context_.float_freereg1_ == tac->a_) {
          emitln("vmov " + FloatRegIDToName(0) + ", " + FloatRegIDToName(valreg));
        } else if (func_context_.float_freereg2_ == tac->a_) {
          emitln("vmov " + FloatRegIDToName(func_context_.func_attr_.attr.used_regs.floatReservedReg) + ", " +
                 FloatRegIDToName(valreg));
        }

        emitln("vstr " + FloatRegIDToName(valreg) + ", [" + IntRegIDToName(dstreg) + "]");
      } else {
        valreg = alloc_reg(tac->b_, dstreg);
        if (func_context_.int_freereg1_ == tac->a_) {
          emitln("mov " + IntRegIDToName(0) + ", " + IntRegIDToName(valreg));
        } else if (func_context_.int_freereg2_ == tac->a_) {
          emitln("mov " + IntRegIDToName(func_context_.func_attr_.attr.used_regs.intReservedReg) + ", " +
                 IntRegIDToName(valreg));
        }
        emitln("str " + IntRegIDToName(valreg) + ", [" + IntRegIDToName(dstreg) + "]");
      }
    } else {
      int valreg = alloc_reg(tac->b_);
      int dstreg = alloc_reg(tac->a_, valreg, true);
      if (tac->b_->value_.UnderlyingType() == SymbolValue::ValueType::Float) {
        emitln("vmov.f32 " + FloatRegIDToName(dstreg) + ", " + FloatRegIDToName(valreg));
      } else {
        emitln("mov " + IntRegIDToName(dstreg) + ", " + IntRegIDToName(valreg));
      }
    }
  };

  auto branch = [&, this]() -> void {
    evit_all_freereg();
    if (tac->operation_ == TACOperationType::Goto) {
      if (tac->a_->type_ != SymbolType::Label) {
        throw std::logic_error("Must goto a label");
      }
      emitln("b " + tac->a_->get_tac_name(true));
    } else {
      //是IfZero
      // a是label b是cond
      int valreg = alloc_reg(tac->b_);
      if (tac->b_->value_.UnderlyingType() == SymbolValue::ValueType::Float) {
        emitln("vcmp.f32 " + FloatRegIDToName(valreg) + ", #0");
        emitln("vmrs APSR_nzcv, FPSCR");
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
              emitln("vmov.f32 " + FloatRegIDToName(symAttr.value) + ", s0");
            }
            break;
          } else if (symAttr.value != offset) {
            throw std::runtime_error("Move register parameter to another register");
          }
          break;
        }
        int32_t realoffset = func_context_.stack_size_for_vars_ + func_context_.stack_size_for_regsave_ + offset;
        if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
          emitln("vldr " + FloatRegIDToName(symAttr.value) + ", [sp, #" + std::to_string(realoffset) + "]");
        } else {
          int intReservedReg = func_context_.func_attr_.attr.used_regs.intReservedReg;
          emitln("ldr " + IntRegIDToName(intReservedReg) + ", =" + std::to_string(realoffset));
          emitln("add " + IntRegIDToName(intReservedReg) + ", " + IntRegIDToName(intReservedReg) + ", sp");
          emitln("vldr " + FloatRegIDToName(symAttr.value) + ", [" + IntRegIDToName(intReservedReg) + "]");
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
            emitln("vstr " + FloatRegIDToName(offset) + ", [sp, #" + std::to_string(realoffset) + "]");
          } else {
            emitln("str " + IntRegIDToName(offset) + ", [sp, #" + std::to_string(realoffset) + "]");
          }
        } else {
          if (is_float) {
            int intReservedReg = func_context_.func_attr_.attr.used_regs.intReservedReg;
            emitln("ldr " + IntRegIDToName(intReservedReg) + ", =" + std::to_string(realoffset));
            emitln("add " + IntRegIDToName(intReservedReg) + ", " + IntRegIDToName(intReservedReg) + ", sp");
            emitln("vstr " + FloatRegIDToName(offset) + ", [" + IntRegIDToName(intReservedReg) + "]");
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

  auto do_call = [&, this]() -> void {
    //初始化一下
    func_context_.stack_size_for_args_ = 0;
    evit_all_freereg();
    //先压栈吧
    emitln("push {r1-r3}");
    emitln("vpush {s1-s15}");
    //全压！这是刚才压了的寄存器的大小
    int save_reg_size = 3 * 4 + 15 * 4;
    //如果是库函数，保存一下ip
    if (!tac->b_->IsGlobal()) {
      emitln("push {ip}");
      save_reg_size += 4;
    }
    func_context_.stack_size_for_args_ += save_reg_size;
    int nstackarg = 0;
    for (auto &record : func_context_.arg_records_) {
      if (!record.storage_in_reg) {
        nstackarg++;
      }
    }
    //计算一下总大小
    size_t sumstack = 0;
    bool padding = false;
    sumstack += nstackarg * 4;
    sumstack += func_context_.stack_size_for_args_;
    sumstack += func_context_.stack_size_for_regsave_;
    sumstack += func_context_.stack_size_for_vars_;
    //因为让栈8对齐，这里检查一下并对齐
    if (sumstack % 8 != 0) {
      padding = true;
      emitln("sub sp, sp, #4");
      func_context_.stack_size_for_args_ += 4;
    }

    //先把所有栈上的压了，反向压栈。
    for (auto it = func_context_.arg_records_.rbegin(); it != func_context_.arg_records_.rend(); ++it) {
      if (it->storage_in_reg) {
        continue;
      }
      if (it->isaddr) {
        //一定是数组才能取地址
        assert(it->sym->value_.Type() == SymbolValue::ValueType::Array);
        auto arrayDescriptor = it->sym->value_.GetArrayDescriptor();
        auto basesym = arrayDescriptor->base_addr.lock();
        auto offsym = arrayDescriptor->base_offset;
        int basereg = alloc_reg(basesym);
        int offreg = alloc_reg(offsym, basereg);
        //如果basesym分配在栈空间，result可以复用basereg
        if (symbol_reg(basesym) == -1) {
          emitln("add " + IntRegIDToName(basereg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg) +
                 ", LSL #2");
          emitln("push {" + IntRegIDToName(basereg) + "}");
          func_context_.stack_size_for_args_ += 4;
          if (basereg == 0) {
            // assert(func_context_.int_freereg1_ == basesym);
            func_context_.int_freereg1_ = nullptr;
          } else {
            // assert(func_context_.int_freereg2_ == basesym);
            func_context_.int_freereg2_ = nullptr;
          }
        } else {
          //否则basesym分配在寄存器，需要再申请一个reg储存结果
          int resreg = (offreg == 0 ? func_context_.func_attr_.attr.used_regs.intReservedReg : 0);
          evit_int_reg(resreg);
          emitln("add " + IntRegIDToName(resreg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg) +
                 ", LSL #2");
          emitln("push {" + IntRegIDToName(resreg) + "}");
          func_context_.stack_size_for_args_ += 4;
        }
      } else {
        int reg = alloc_reg(it->sym);
        if (it->sym->value_.Type() == SymbolValue::ValueType::Float) {
          emitln("vpush {" + FloatRegIDToName(reg) + "}");
          func_context_.stack_size_for_args_ += 4;
        } else {
          emitln("push {" + IntRegIDToName(reg) + "}");
          func_context_.stack_size_for_args_ += 4;
        }
      }
    }

    //然后处理寄存器
    //先让float的全压了，因为是倒序，保证了s0最后处理
    for (auto it = func_context_.arg_records_.rbegin(); it != func_context_.arg_records_.rend(); ++it) {
      if (!it->storage_in_reg) {
        continue;
      }
      if (it->sym->value_.Type() != SymbolValue::ValueType::Float) {
        continue;
      }
      int reg = symbol_reg(it->sym);
      if (reg == -1) {
        reg = alloc_reg(it->sym);
      }
      //需要特殊处理会被寄存器传参用到的寄存器位置的变量
      if (reg > 0 && reg < 16) {
        //从栈上定位一下
        int regoffset;
        {
          //压栈的顺序是倒过来的，所以是16 - reg ，前面的3是因为前面有3个通用寄存器压栈
          int nth = 3 + 16 - reg;
          regoffset = func_context_.stack_size_for_args_ - nth * 4;
        }
        if (ArmHelper::IsLDRSTRImmediateValue(regoffset)) {
          emitln("vldr " + FloatRegIDToName(it->storage_pos) + ", [sp, #" + std::to_string(regoffset) + "]");
        } else {
          auto divides = ArmHelper::DivideIntoImmediateValues(regoffset);
          for (auto val : divides) {
            emitln("add sp, sp, #" + std::to_string(val));
          }
          emitln("vldr " + FloatRegIDToName(it->storage_pos) + ", [sp]");
          for (auto val : divides) {
            emitln("sub sp, sp, #" + std::to_string(val));
          }
        }
      } else if (it->storage_pos != reg) {
        emitln("vmov " + FloatRegIDToName(it->storage_pos) + ", " + FloatRegIDToName(reg));
      }
    }
    auto read_intreg_from_stack = [&, this](int src_regid, int dst_regid) -> void {
      //一定要是栈上的才行
      assert(src_regid > 0 && src_regid < 4);
      //压栈的顺序是倒过来的，所以是4 - reg
      int nth = 4 - src_regid;
      int offset = func_context_.stack_size_for_args_ - nth * 4;
      if (ArmHelper::IsLDRSTRImmediateValue(offset)) {
        emitln("ldr " + IntRegIDToName(dst_regid) + ", [sp, #" + std::to_string(offset) + "]");
      } else {
        emitln("ldr " + IntRegIDToName(dst_regid) + ", =" + std::to_string(offset));
        emitln("ldr " + IntRegIDToName(dst_regid) + ", [sp, " + IntRegIDToName(dst_regid) + "]");
      }
    };
    for (auto it = func_context_.arg_records_.rbegin(); it != func_context_.arg_records_.rend(); ++it) {
      if (!it->storage_in_reg) {
        continue;
      }
      if (it->sym->value_.Type() == SymbolValue::ValueType::Float) {
        continue;
      }
      if (it->isaddr) {
        assert(it->sym->value_.Type() == SymbolValue::ValueType::Array);
        auto arrayDescriptor = it->sym->value_.GetArrayDescriptor();
        auto basesym = arrayDescriptor->base_addr.lock();
        auto offsym = arrayDescriptor->base_offset;
        int basereg = symbol_reg(basesym);
        bool basesym_on_stack = false;
        if (basereg == -1) {
          basesym_on_stack = true;
          basereg = alloc_reg(it->sym);
        }
        int offreg = symbol_reg(offsym);
        if (offreg == -1) {
          offreg = alloc_reg(offsym, basereg);
        }
        //如果basesym分配在栈空间，result可以复用basereg
        if (basesym_on_stack) {
          //处理一下offreg从栈中读取
          if (0 < offreg && offreg < 4) {
            evit_int_reg(basereg == 0);
            int freeintreg = get_free_int_reg();
            read_intreg_from_stack(offreg, freeintreg);
            offreg = freeintreg;
          }
          emitln("add " + IntRegIDToName(basereg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg) +
                 ", LSL #2");
          if (basereg != it->storage_pos) {
            emitln("mov " + IntRegIDToName(it->storage_pos) + ", " + IntRegIDToName(basereg));
          }
          if (basereg == 0) {
            // assert(func_context_.int_freereg1_ == basesym);
            func_context_.int_freereg1_ = nullptr;
          } else {
            // assert(func_context_.int_freereg2_ == basesym);
            func_context_.int_freereg2_ = nullptr;
          }
        } else {
          //否则basesym分配在寄存器，需要再申请一个reg储存结果
          int resreg = (offreg == 0 ? func_context_.func_attr_.attr.used_regs.intReservedReg : 0);
          evit_int_reg(resreg);

          //处理一下basereg从栈中读取
          if (0 < basereg && basereg < 4) {
            read_intreg_from_stack(basereg, resreg);
            basereg = resreg;
          }

          //处理一下offreg从栈中读取
          if (0 < offreg && offreg < 4) {
            evit_int_reg(resreg == 0);
            int freeintreg = get_free_int_reg();
            read_intreg_from_stack(offreg, freeintreg);
            offreg = freeintreg;
          }

          emitln("add " + IntRegIDToName(resreg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg) +
                 ", LSL #2");
          if (resreg != it->storage_pos) {
            emitln("mov " + IntRegIDToName(it->storage_pos) + ", " + IntRegIDToName(resreg));
          }
        }
      } else {
        int reg = symbol_reg(it->sym);
        if (reg == -1) {
          reg = alloc_reg(it->sym);
        }
        //需要特殊处理会被寄存器传参用到的寄存器位置的变量
        if (reg > 0 && reg < 4) {
          read_intreg_from_stack(reg, it->storage_pos);
        } else if (reg != it->storage_pos) {
          emitln("mov " + IntRegIDToName(it->storage_pos) + ", " + IntRegIDToName(reg));
        }
      }
    }

    //可以call了
    evit_all_freereg();
    emitln("bl " + tac->b_->get_tac_name(true));

    //去掉所有栈上arg
    uint32_t toaddstack = padding ? 4 : 0;
    toaddstack += nstackarg * 4;
    if (!ArmHelper::EmitImmediateInstWithCheck(emitln, "add", "sp", "sp", toaddstack)) {
      auto splitaddstack = ArmHelper::DivideIntoImmediateValues(toaddstack);
      for (auto value : splitaddstack) {
        emitln("add sp, sp, #" + std::to_string(value));
      }
    }
    func_context_.stack_size_for_args_ -= toaddstack;

    //恢复寄存器

    //如果是库函数，还原一下ip
    if (!tac->b_->IsGlobal()) {
      emitln("pop {ip}");
    }

    emitln("vpop {s1-s15}");
    emitln("pop {r1-r3}");

    func_context_.stack_size_for_args_ -= save_reg_size;

    assert(func_context_.stack_size_for_args_ == 0);

    //如果有接收变量
    if (tac->a_ != nullptr) {
      //返回在s0
      if (tac->a_->value_.Type() == SymbolValue::ValueType::Float) {
        //如果在栈上，我们直接认为s0处是它就可以了
        int regid = symbol_reg(tac->a_);
        if (regid == -1) {
          func_context_.float_freereg1_ = tac->a_;
        } else {
          //否则mov过去就好
          emitln("vmov " + FloatRegIDToName(regid) + ", s0");
        }
      } else {
        //同上
        int regid = symbol_reg(tac->a_);
        if (regid == -1) {
          func_context_.int_freereg1_ = tac->a_;
        } else {
          emitln("mov " + IntRegIDToName(regid) + ", r0");
        }
      }
    }
    func_context_.arg_nfloatregs_ = 0;
    func_context_.arg_nintregs_ = 0;
    func_context_.arg_stacksize_ = 0;
    func_context_.arg_records_.clear();
  };

  auto do_return = [&, this]() -> void {
    evit_all_freereg();
    if (tac->a_ != nullptr) {
      //设置返回值
      int retreg = alloc_reg(tac->a_);
      //为0的话就不用倒腾了
      if (retreg != 0) {
        if (tac->a_->value_.Type() == SymbolValue::ValueType::Float) {
          emitln("vmov s0, " + FloatRegIDToName(retreg));
        } else {
          emitln("mov r0, " + IntRegIDToName(retreg));
        }
      }
      func_context_.int_freereg1_ = nullptr;
      func_context_.int_freereg2_ = nullptr;
      func_context_.float_freereg1_ = nullptr;
      func_context_.float_freereg2_ = nullptr;
    }

    //释放变量栈空间
    for (auto immval : func_context_.var_stack_immvals_) {
      bool check = ArmHelper::EmitImmediateInstWithCheck(emitln, "add", "sp", "sp", immval);
      assert(check);
    }

    //还原寄存器
    //还原浮点寄存器
    for (auto regid : func_context_.savefloatregs_) {
      if (regid.first == regid.second) {
        emitln("vpop { " + FloatRegIDToName(regid.first) + " }");
      } else {
        emitln("vpop { " + FloatRegIDToName(regid.first) + "-" + FloatRegIDToName(regid.second) + " }");
      }
    }

    //还原通用寄存器
    bool pop_pc = false;
    if (Contains(func_context_.saveintregs_, LR_REGID) && !Contains(func_context_.saveintregs_, PC_REGID)) {
      pop_pc = true;
    }
    if (!func_context_.saveintregs_.empty()) {
      std::string inst = "pop {";
      for (size_t i = 0; i < func_context_.saveintregs_.size(); i++) {
        int regid = func_context_.saveintregs_[i];
        if (pop_pc && regid == LR_REGID) {
          regid = PC_REGID;
        }
        inst += IntRegIDToName(regid);
        if (i + 1 < func_context_.saveintregs_.size()) {
          inst += ", ";
        }
      }
      inst += "}";
      emitln(inst);
    }
    if (!pop_pc) {
      //没有用栈来pop lr到sp位置
      emitln("bx lr");
    }
  };

  auto do_call_return = [&, this]() -> void {
    //初始化一下
    func_context_.stack_size_for_args_ = 0;
    evit_all_freereg();
    //先压栈吧
    emitln("push {r1-r3}");
    emitln("vpush {s1-s15}");
    //全压！这是刚才压了的寄存器的大小
    int save_reg_size = 3 * 4 + 15 * 4;
    
    func_context_.stack_size_for_args_ += save_reg_size;
    int nstackarg = 0;
    for (auto &record : func_context_.arg_records_) {
      if (!record.storage_in_reg) {
        nstackarg++;
      }
    }
    // //计算一下总大小
    // size_t sumstack = 0;
    // bool padding = false;
    // sumstack += nstackarg * 4;
    // sumstack += func_context_.stack_size_for_args_;
    // sumstack += func_context_.stack_size_for_regsave_;
    // sumstack += func_context_.stack_size_for_vars_;
    // //因为让栈8对齐，这里检查一下并对齐
    // if (sumstack % 8 != 0) {
    //   padding = true;
    //   emitln("sub sp, sp, #4");
    //   func_context_.stack_size_for_args_ += 4;
    // }
    //记录一下栈开头位置
    int stack_start_of_args = func_context_.stack_size_for_args_;

    //先把所有栈上的压了，反向压栈。
    for (auto it = func_context_.arg_records_.rbegin(); it != func_context_.arg_records_.rend(); ++it) {
      if (it->storage_in_reg) {
        continue;
      }
      if (it->isaddr) {
        //一定是数组才能取地址
        assert(it->sym->value_.Type() == SymbolValue::ValueType::Array);
        auto arrayDescriptor = it->sym->value_.GetArrayDescriptor();
        auto basesym = arrayDescriptor->base_addr.lock();
        auto offsym = arrayDescriptor->base_offset;
        int basereg = alloc_reg(basesym);
        int offreg = alloc_reg(offsym, basereg);
        //如果basesym分配在栈空间，result可以复用basereg
        if (symbol_reg(basesym) == -1) {
          emitln("add " + IntRegIDToName(basereg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg) +
                 ", LSL #2");
          emitln("push {" + IntRegIDToName(basereg) + "}");
          func_context_.stack_size_for_args_ += 4;
          if (basereg == 0) {
            // assert(func_context_.int_freereg1_ == basesym);
            func_context_.int_freereg1_ = nullptr;
          } else {
            // assert(func_context_.int_freereg2_ == basesym);
            func_context_.int_freereg2_ = nullptr;
          }
        } else {
          //否则basesym分配在寄存器，需要再申请一个reg储存结果
          int resreg = (offreg == 0 ? func_context_.func_attr_.attr.used_regs.intReservedReg : 0);
          evit_int_reg(resreg);
          emitln("add " + IntRegIDToName(resreg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg) +
                 ", LSL #2");
          emitln("push {" + IntRegIDToName(resreg) + "}");
          func_context_.stack_size_for_args_ += 4;
        }
      } else {
        int reg = alloc_reg(it->sym);
        if (it->sym->value_.Type() == SymbolValue::ValueType::Float) {
          emitln("vpush {" + FloatRegIDToName(reg) + "}");
          func_context_.stack_size_for_args_ += 4;
        } else {
          emitln("push {" + IntRegIDToName(reg) + "}");
          func_context_.stack_size_for_args_ += 4;
        }
      }
    }

    //然后处理寄存器
    //先让float的全压了，因为是倒序，保证了s0最后处理
    for (auto it = func_context_.arg_records_.rbegin(); it != func_context_.arg_records_.rend(); ++it) {
      if (!it->storage_in_reg) {
        continue;
      }
      if (it->sym->value_.Type() != SymbolValue::ValueType::Float) {
        continue;
      }
      int reg = symbol_reg(it->sym);
      if (reg == -1) {
        reg = alloc_reg(it->sym);
      }
      //需要特殊处理会被寄存器传参用到的寄存器位置的变量
      if (reg > 0 && reg < 16) {
        //从栈上定位一下
        int regoffset;
        {
          //压栈的顺序是倒过来的，所以是16 - reg ，前面的3是因为前面有3个通用寄存器压栈
          int nth = 3 + 16 - reg;
          regoffset = func_context_.stack_size_for_args_ - nth * 4;
        }
        if (ArmHelper::IsLDRSTRImmediateValue(regoffset)) {
          emitln("vldr " + FloatRegIDToName(it->storage_pos) + ", [sp, #" + std::to_string(regoffset) + "]");
        } else {
          auto divides = ArmHelper::DivideIntoImmediateValues(regoffset);
          for (auto val : divides) {
            emitln("add sp, sp, #" + std::to_string(val));
          }
          emitln("vldr " + FloatRegIDToName(it->storage_pos) + ", [sp]");
          for (auto val : divides) {
            emitln("sub sp, sp, #" + std::to_string(val));
          }
        }
      } else if (it->storage_pos != reg) {
        emitln("vmov " + FloatRegIDToName(it->storage_pos) + ", " + FloatRegIDToName(reg));
      }
    }
    auto read_intreg_from_stack = [&, this](int src_regid, int dst_regid) -> void {
      //一定要是栈上的才行
      assert(src_regid > 0 && src_regid < 4);
      //压栈的顺序是倒过来的，所以是4 - reg
      int nth = 4 - src_regid;
      int offset = func_context_.stack_size_for_args_ - nth * 4;
      if (ArmHelper::IsLDRSTRImmediateValue(offset)) {
        emitln("ldr " + IntRegIDToName(dst_regid) + ", [sp, #" + std::to_string(offset) + "]");
      } else {
        emitln("ldr " + IntRegIDToName(dst_regid) + ", =" + std::to_string(offset));
        emitln("ldr " + IntRegIDToName(dst_regid) + ", [sp, " + IntRegIDToName(dst_regid) + "]");
      }
    };
    for (auto it = func_context_.arg_records_.rbegin(); it != func_context_.arg_records_.rend(); ++it) {
      if (!it->storage_in_reg) {
        continue;
      }
      if (it->sym->value_.Type() == SymbolValue::ValueType::Float) {
        continue;
      }
      if (it->isaddr) {
        assert(it->sym->value_.Type() == SymbolValue::ValueType::Array);
        auto arrayDescriptor = it->sym->value_.GetArrayDescriptor();
        auto basesym = arrayDescriptor->base_addr.lock();
        auto offsym = arrayDescriptor->base_offset;
        int basereg = symbol_reg(basesym);
        bool basesym_on_stack = false;
        if (basereg == -1) {
          basesym_on_stack = true;
          basereg = alloc_reg(it->sym);
        }
        int offreg = symbol_reg(offsym);
        if (offreg == -1) {
          offreg = alloc_reg(offsym, basereg);
        }
        //如果basesym分配在栈空间，result可以复用basereg
        if (basesym_on_stack) {
          //处理一下offreg从栈中读取
          if (0 < offreg && offreg < 4) {
            evit_int_reg(basereg == 0);
            int freeintreg = get_free_int_reg();
            read_intreg_from_stack(offreg, freeintreg);
            offreg = freeintreg;
          }
          emitln("add " + IntRegIDToName(basereg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg) +
                 ", LSL #2");
          if (basereg != it->storage_pos) {
            emitln("mov " + IntRegIDToName(it->storage_pos) + ", " + IntRegIDToName(basereg));
          }
          if (basereg == 0) {
            // assert(func_context_.int_freereg1_ == basesym);
            func_context_.int_freereg1_ = nullptr;
          } else {
            // assert(func_context_.int_freereg2_ == basesym);
            func_context_.int_freereg2_ = nullptr;
          }
        } else {
          //否则basesym分配在寄存器，需要再申请一个reg储存结果
          int resreg = (offreg == 0 ? func_context_.func_attr_.attr.used_regs.intReservedReg : 0);
          evit_int_reg(resreg);

          //处理一下basereg从栈中读取
          if (0 < basereg && basereg < 4) {
            read_intreg_from_stack(basereg, resreg);
            basereg = resreg;
          }

          //处理一下offreg从栈中读取
          if (0 < offreg && offreg < 4) {
            evit_int_reg(resreg == 0);
            int freeintreg = get_free_int_reg();
            read_intreg_from_stack(offreg, freeintreg);
            offreg = freeintreg;
          }

          emitln("add " + IntRegIDToName(resreg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg) +
                 ", LSL #2");
          if (resreg != it->storage_pos) {
            emitln("mov " + IntRegIDToName(it->storage_pos) + ", " + IntRegIDToName(resreg));
          }
        }
      } else {
        int reg = symbol_reg(it->sym);
        if (reg == -1) {
          reg = alloc_reg(it->sym);
        }
        //需要特殊处理会被寄存器传参用到的寄存器位置的变量
        if (reg > 0 && reg < 4) {
          read_intreg_from_stack(reg, it->storage_pos);
        } else if (reg != it->storage_pos) {
          emitln("mov " + IntRegIDToName(it->storage_pos) + ", " + IntRegIDToName(reg));
        }
      }
    }

    //挪一下栈。
    func_context_.int_freereg1_ = nullptr;
    func_context_.int_freereg2_ = nullptr;
    func_context_.float_freereg1_ = nullptr;
    func_context_.float_freereg2_ = nullptr;

    //把当前所有栈都退掉，推到本函数保存寄存器处
    {
      int totalsize = func_context_.stack_size_for_args_ + func_context_.stack_size_for_vars_;
      if (!ArmHelper::EmitImmediateInstWithCheck(emitln, "add", "sp", "sp", totalsize)) {
        auto pieces = ArmHelper::DivideIntoImmediateValues(totalsize);
        for (auto val : pieces) {
          emitln("add sp, sp, #" + std::to_string(val));
        }
      }
    }

    //还原寄存器
    //还原浮点寄存器
    for (auto regid : func_context_.savefloatregs_) {
      if (regid.first == regid.second) {
        emitln("vpop { " + FloatRegIDToName(regid.first) + " }");
      } else {
        emitln("vpop { " + FloatRegIDToName(regid.first) + "-" + FloatRegIDToName(regid.second) + " }");
      }
    }

    //还原通用寄存器
    if (!func_context_.saveintregs_.empty()) {
      std::string inst = "pop {";
      for (size_t i = 0; i < func_context_.saveintregs_.size(); i++) {
        inst += IntRegIDToName(func_context_.saveintregs_[i]);
        if (i + 1 < func_context_.saveintregs_.size()) {
          inst += ", ";
        }
      }
      inst += "}";
      emitln(inst);
    }

    // bool padding = false;
    //回到函数参数前端，留三位置放寄存器
    {
      stack_start_of_args -= 3 * 4;
      //对齐一下
      if ((func_context_.stack_size_for_args_ - stack_start_of_args) % 8 != 0) {
        // padding = true;
        stack_start_of_args -= 4;
      }
      int totalsize = func_context_.stack_size_for_regsave_ + func_context_.stack_size_for_vars_ + stack_start_of_args;
      if (!ArmHelper::EmitImmediateInstWithCheck(emitln, "sub", "sp", "sp", totalsize)) {
        auto pieces = ArmHelper::DivideIntoImmediateValues(totalsize);
        for (auto val : pieces) {
          emitln("sub sp, sp, #" + std::to_string(val));
        }
      }
    }
    //保存三寄存器用来临时倒腾
    emitln("push {fp, ip, lr}");
    //重新回到顶部
    emitln("add sp, sp, #12");

    //这是真实参数栈大小
    int real_stack_args_size = (int)func_context_.stack_size_for_args_ - stack_start_of_args;


    //现在开始挪动，可以自由使用ip和fp寄存器
    int count = real_stack_args_size / 4;
    int move_distance =
        (int)func_context_.stack_size_for_regsave_ + (int)func_context_.stack_size_for_vars_ + stack_start_of_args;

    //将fp置为本函数开头位置
    if (!ArmHelper::EmitImmediateInstWithCheck(emitln, "add", "fp", "sp", move_distance)) {
      auto pieces = ArmHelper::DivideIntoImmediateValues(move_distance);
      for (auto val : pieces) {
        emitln("add fp, sp, #" + std::to_string(val));
      }
    }
    if (count > 0) {
      emitln("ldr lr, =" + std::to_string(count));
      std::string loop_label = "_call_ret_loop_" + std::to_string(++this->counter);
      emitln(loop_label + ":");
      emitln("ldr ip, [sp, #-4]!");
      emitln("str ip, [fp, #-4]!");
      emitln("subs lr, #1");
      emitln("bgt " + loop_label);
    }
    emitln("mov sp, fp");

    //可以call了
    emitln("bl " + tac->b_->get_tac_name(true));

    //现在把sp放到参数前
    {
      int delta_distance = real_stack_args_size - 12;
      if (!ArmHelper::EmitImmediateInstWithCheck(emitln, "add", "sp", "sp", delta_distance)) {
        emitln("ldr lr, =" + std::to_string(delta_distance));
        emitln("add sp, sp, lr");
      }
    }

    // pop pc 代替 pop lr  然后 bx lr
    emitln("pop {fp, ip, pc}");

    // emitln("bx lr");

    func_context_.arg_nfloatregs_ = 0;
    func_context_.arg_nintregs_ = 0;
    func_context_.arg_stacksize_ = 0;
    func_context_.arg_records_.clear();    
  };

  auto functionality = [&, this]() -> void {
    switch (tac->operation_) {
      case TACOperationType::Argument:
      case TACOperationType::ArgumentAddress: {
        ArmUtil::FunctionContext::ArgRecord record;
        //浮点类型
        if (tac->operation_ == TACOperationType::Argument &&
            tac->a_->value_.UnderlyingType() == SymbolValue::ValueType::Float) {
          //如果小于16，可以用
          if (func_context_.arg_nfloatregs_ < 16) {
            record.storage_in_reg = true;
            record.storage_pos = func_context_.arg_nfloatregs_;
            func_context_.arg_nfloatregs_++;
          } else {
            //否则存栈上
            record.storage_in_reg = false;
            record.storage_pos = func_context_.arg_stacksize_;
            func_context_.arg_stacksize_ += 4;
          }
        } else {
          if (func_context_.arg_nintregs_ < 4) {
            record.storage_in_reg = true;
            record.storage_pos = func_context_.arg_nintregs_;
            func_context_.arg_nintregs_++;
          } else {
            //否则存栈上
            record.storage_in_reg = false;
            record.storage_pos = func_context_.arg_stacksize_;
            func_context_.arg_stacksize_ += 4;
          }
        }
        if (tac->operation_ == TACOperationType::ArgumentAddress) {
          record.isaddr = true;
        } else {
          record.isaddr = false;
        }
        record.sym = tac->a_;
        func_context_.arg_records_.push_back(record);
        break;
      }
      case TACOperationType::Call: {
        do_call();
        break;
      }

      case TACOperationType::Return: {
        do_return();
        break;
      }
      default:
        throw std::logic_error("Unreachable");
    }
  };

  auto label_declaration = [&, this]() -> void {
    evit_all_freereg();
    emitln(tac->a_->get_tac_name(true) + ":");
  };

  auto array_declaration = [&, this]() -> void {
    auto arrayAttr = func_context_.reg_alloc_->get_ArrayAttribute(tac->a_);
    int reg = alloc_reg(tac->a_);
    int32_t realoffset = arrayAttr.value + func_context_.stack_size_for_args_;
    if (!ArmHelper::EmitImmediateInstWithCheck(emitln, "add", IntRegIDToName(reg), "sp", realoffset)) {
      emitln("ldr " + IntRegIDToName(reg) + ", =" + std::to_string(realoffset));
      emitln("add " + IntRegIDToName(reg) + ", sp, " + IntRegIDToName(reg));
    }
  };

  auto cast_operation = [&, this]() -> void {
    bool afloat = (tac->a_->value_.Type() == SymbolValue::ValueType::Float);
    bool bfloat = (tac->b_->value_.Type() == SymbolValue::ValueType::Float);
    assert(afloat != bfloat);
    if (tac->operation_ == TACOperationType::FloatToInt) {
      assert(bfloat && !afloat);
      int freefloatreg = get_free_float_reg();
      int breg = alloc_reg(tac->b_, freefloatreg);
      int areg = alloc_reg(tac->a_);
      emitln("vcvt.s32.f32 " + FloatRegIDToName(freefloatreg) + ", " + FloatRegIDToName(breg));
      emitln("vmov " + IntRegIDToName(areg) + ", " + FloatRegIDToName(freefloatreg));
    } else {
      assert(!bfloat && afloat);
      int freefloatreg = get_free_float_reg();
      int breg = alloc_reg(tac->b_, freefloatreg);
      int areg = alloc_reg(tac->a_);
      emitln("vmov " + FloatRegIDToName(freefloatreg) + ", " + IntRegIDToName(breg));
      emitln("vcvt.f32.s32 " + FloatRegIDToName(areg) + ", " + FloatRegIDToName(freefloatreg));
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
    case TACOperationType::IntToFloat:
    case TACOperationType::FloatToInt:
      func_context_.parameter_head_ = false;
      cast_operation();
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
    case TACOperationType::CallAndReturn:
      func_context_.parameter_head_ = false;
      do_call_return();
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