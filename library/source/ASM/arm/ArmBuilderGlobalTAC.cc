#include <iostream>
#include <string>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/arm/ArmBuilder.hh"
#include "ASM/arm/ArmHelper.hh"
#include "ASM/arm/RegAllocator.hh"
#include "MagicEnum.hh"
#include "TAC/TAC.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {
using namespace HaveFunCompiler::ThreeAddressCode;

std::string ArmBuilder::GlobalTACToASMString([[maybe_unused]] TACPtr tac) {
  std::string ret = "";
  auto emitln = [&ret](const std::string &inst) -> void {
    ret.append(inst);
    ret.append("\n");
  };
  //来个注释好了
  emitln("// " + tac->ToString());

  //获得相对于当前sp的地址
  auto getrealoffset = [&, this](SymbolPtr sym) -> int32_t {
    //只给临时变量分配就可以
    assert(!sym->IsLiteral());
    assert(sym->IsGlobalTemp());
    auto it = glob_context_.var_stack_pos.find(sym);
    //之前一定分配好了
    if (it == glob_context_.var_stack_pos.end()) {
      throw std::logic_error("Getrealoffset: not allocated tmp var " + sym->get_name());
    }
    int off = it->second;
    int othersize = glob_context_.stack_size_for_args_;
    return othersize + off;
  };

  // 0<=reg_id<USE_INT_REG_NUM 驱逐指定reg_id
  auto evit_int_reg = [&, this](int reg_id) -> void {
    if (reg_id < 0 || reg_id >= glob_context_.USE_INT_REG_NUM) {
      throw std::logic_error("Int reg id is out of range");
    }
    SymbolPtr &target_sym = glob_context_.int_regs_[reg_id];
    if (target_sym == nullptr) {
      //不需要驱逐
      return;
    }
    if (target_sym->IsLiteral()) {
      //不需要回存
      target_sym = nullptr;
      return;
    }
    //是全局变量
    assert(target_sym->IsGlobal());

    if (!target_sym->IsGlobalTemp()) {
      //数组指针不会被更改，直接就不用会存。
      if (target_sym->value_.Type() != SymbolValue::ValueType::Array) {
        //用了一个正常时候用不到的寄存器当临时寄存器。
        int otherreg = glob_context_.USE_INT_REG_NUM;
        emitln("ldr " + IntRegIDToName(otherreg) + ", " + ToDataRefName(GetVariableName(target_sym)));
        emitln("str " + IntRegIDToName(reg_id) + ", [" + IntRegIDToName(otherreg) + "]");
      }
      target_sym = nullptr;
      return;
    }

    int realoffset = getrealoffset(target_sym);
    if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
      emitln("str " + IntRegIDToName(reg_id) + ", [sp, #" + std::to_string(realoffset) + "]");
    } else {
      //如果不能直接做，借用ip来完成
      int otherreg = glob_context_.USE_INT_REG_NUM;
      emitln("ldr " + IntRegIDToName(otherreg) + ", =" + std::to_string(realoffset));
      emitln("str " + IntRegIDToName(reg_id) + ", [sp, " + IntRegIDToName(otherreg) + "]");
    }
    target_sym = nullptr;
  };

  auto evit_float_reg = [&, this](int reg_id) -> void {
    if (reg_id < 0 || reg_id >= glob_context_.USE_FLOAT_REG_NUM) {
      throw std::logic_error("Float reg id is out of range");
    }
    SymbolPtr &target_sym = glob_context_.float_regs_[reg_id];
    if (target_sym == nullptr) {
      //不需要驱逐
      return;
    }
    if (target_sym->IsLiteral()) {
      //不需要回存
      target_sym = nullptr;
      return;
    }
    //是全局变量
    assert(target_sym->IsGlobal());

    if (!target_sym->IsGlobalTemp()) {
      //用了一个正常时候用不到的寄存器当临时寄存器。
      int otherreg = glob_context_.USE_INT_REG_NUM;
      emitln("ldr " + IntRegIDToName(otherreg) + ", " + ToDataRefName(GetVariableName(target_sym)));
      emitln("vstr " + FloatRegIDToName(reg_id) + ", [" + IntRegIDToName(otherreg) + "]");
      target_sym = nullptr;
      return;
    }

    int realoffset = getrealoffset(target_sym);
    if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
      emitln("vstr " + IntRegIDToName(reg_id) + ", [sp, #" + std::to_string(realoffset) + "]");
    } else {
      //如果不能直接做，借用ip来完成
      int otherreg = glob_context_.USE_INT_REG_NUM;
      emitln("ldr " + IntRegIDToName(otherreg) + ", =" + std::to_string(realoffset));
      emitln("vstr " + FloatRegIDToName(reg_id) + ", [sp, " + IntRegIDToName(otherreg) + "]");
    }
    target_sym = nullptr;
  };

  auto get_free_int_reg = [&, this](int except_reg = -1) -> int {
    for (int i = glob_context_.USE_INT_REG_NUM - 1; i >= 0; i--) {
      if (glob_context_.int_regs_[i] == nullptr && i != except_reg) {
        return i;
      }
    }
    for (int i = glob_context_.USE_INT_REG_NUM - 1; i >= 0; i--) {
      if (glob_context_.int_regs_[i]->IsLiteral() && i != except_reg) {
        return i;
      }
    }
    int backpos = glob_context_.USE_INT_REG_NUM - 1;
    if (except_reg == glob_context_.int_regs_ranks_[backpos]) {
      backpos--;
    }
    int back = glob_context_.int_regs_ranks_[backpos];
    for (int i = backpos; i > 0; i--) {
      glob_context_.int_regs_ranks_[i] = glob_context_.int_regs_ranks_[i - 1];
    }
    glob_context_.int_regs_ranks_[0] = back;
    evit_int_reg(back);
    return back;
  };

  auto get_free_float_reg = [&, this](int except_reg = -1) -> int {
    for (int i = glob_context_.USE_FLOAT_REG_NUM - 1; i >= 0; i--) {
      if (glob_context_.float_regs_[i] == nullptr && i != except_reg) {
        return i;
      }
    }
    for (int i = glob_context_.USE_FLOAT_REG_NUM - 1; i >= 0; i--) {
      if (glob_context_.float_regs_[i]->IsLiteral() && i != except_reg) {
        return i;
      }
    }
    int backpos = glob_context_.USE_FLOAT_REG_NUM - 1;
    if (except_reg == glob_context_.float_regs_ranks_[backpos]) {
      backpos--;
    }
    int back = glob_context_.float_regs_ranks_[backpos];
    for (int i = backpos; i > 0; i--) {
      glob_context_.float_regs_ranks_[i] = glob_context_.float_regs_ranks_[i - 1];
    }
    glob_context_.float_regs_ranks_[0] = back;
    evit_float_reg(back);
    return back;
  };

  auto alloc_reg = [&, this](SymbolPtr sym, int except_reg = -1) -> int {
    bool is_float = (sym->value_.Type() == SymbolValue::ValueType::Float);
    if (sym->value_.Type() == SymbolValue::ValueType::Array) {
      sym = sym->value_.GetArrayDescriptor()->base_addr.lock();
    }

    //如果有缓存就直接用
    int regid;
    if (is_float) {
      for (regid = glob_context_.USE_FLOAT_REG_NUM - 1; regid >= 0; regid--) {
        if (glob_context_.float_regs_[regid] == sym) {
          break;
        }
      }
      if (regid > 0) {
        int regidpos;
        for (regidpos = glob_context_.USE_FLOAT_REG_NUM - 1; regidpos >= 0; regidpos--) {
          if (glob_context_.float_regs_ranks_[regidpos] == regid) {
            break;
          }
        }
        assert(regidpos >= 0);
        for (int i = regidpos; i > 0; i--) {
          glob_context_.float_regs_ranks_[i] = glob_context_.float_regs_ranks_[i - 1];
        }
        glob_context_.float_regs_ranks_[0] = regid;
      }
      if (regid < 0) {
        regid = get_free_float_reg(except_reg);
      }
      if (sym->IsLiteral()) {
        int otherreg = glob_context_.USE_INT_REG_NUM;
        emitln("ldr " + IntRegIDToName(otherreg) +
               ", =" + std::to_string(ArmHelper::BitcastToUInt(sym->value_.GetFloat())));
        emitln("vmov " + FloatRegIDToName(regid) + ", " + IntRegIDToName(otherreg));
      } else if (!sym->IsGlobalTemp()) {
        //是全局变量
        int otherreg = glob_context_.USE_INT_REG_NUM;
        emitln("ldr " + IntRegIDToName(otherreg) + ", " + ToDataRefName(GetVariableName(sym)));
        emitln("vldr " + FloatRegIDToName(regid) + ", [" + IntRegIDToName(otherreg) + "]");
      } else {
        //是全局临时变量
        int realoffset = getrealoffset(sym);
        if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
          emitln("vldr s" + std::to_string(regid) + ", [sp, #" + std::to_string(realoffset) + "]");
        } else {
          int otherreg = glob_context_.USE_INT_REG_NUM;
          emitln("ldr " + IntRegIDToName(otherreg) + ", =" + std::to_string(realoffset));
          emitln("vldr " + FloatRegIDToName(regid) + ", [sp, " + IntRegIDToName(otherreg) + "]");
        }
      }
    } else {
      for (regid = glob_context_.USE_INT_REG_NUM - 1; regid >= 0; regid--) {
        if (glob_context_.int_regs_[regid] == sym) {
          break;
        }
      }
      if (regid > 0) {
        int regidpos;
        for (regidpos = glob_context_.USE_INT_REG_NUM - 1; regidpos >= 0; regidpos--) {
          if (glob_context_.int_regs_ranks_[regidpos] == regid) {
            break;
          }
        }
        assert(regidpos >= 0);
        for (int i = regidpos; i > 0; i--) {
          glob_context_.int_regs_ranks_[i] = glob_context_.int_regs_ranks_[i - 1];
        }
        glob_context_.int_regs_ranks_[0] = regid;
      }
      if (regid < 0) {
        regid = get_free_int_reg(except_reg);
      }
      if (sym->IsLiteral()) {
        int val = sym->value_.GetInt();
        if (ArmHelper::IsImmediateValue(val)) {
          emitln("mov " + IntRegIDToName(regid) + ", #" + std::to_string(val));
        } else {
          emitln("ldr " + IntRegIDToName(regid) + ", =" + std::to_string(val));
        }
      } else if (!sym->IsGlobalTemp()) {
        emitln("ldr " + IntRegIDToName(regid) + ", " + ToDataRefName(GetVariableName(sym)));
        if (sym->value_.Type() != SymbolValue::ValueType::Array) {
          emitln("ldr " + IntRegIDToName(regid) + ", [" + IntRegIDToName(regid) + "]");
        }
      } else {
        int32_t realoffset = getrealoffset(sym);
        if (ArmHelper::IsLDRSTRImmediateValue(realoffset)) {
          emitln("ldr " + IntRegIDToName(regid) + ", [sp, #" + std::to_string(realoffset) + "]");
        } else {
          emitln("ldr " + IntRegIDToName(regid) + ", =" + std::to_string(realoffset));
          emitln("ldr " + IntRegIDToName(regid) + ", [sp, " + IntRegIDToName(regid) + "]");
        }
      }
    }
    return regid;
  };

  auto simple_binary_operation = [&, this](std::string op) -> void {
    if (tac->a_->value_.Type() == SymbolValue::ValueType::Float) {
      int op1reg = alloc_reg(tac->b_);
      int op2reg = alloc_reg(tac->c_, op1reg);
      int resreg = alloc_reg(tac->a_, op2reg);
      if (resreg == op1reg || resreg == op2reg) {
        op1reg = alloc_reg(tac->b_);
        op2reg = alloc_reg(tac->c_, op1reg);
        int tmpreg = 0;
        while (tmpreg == op1reg || tmpreg == op2reg) {
          tmpreg++;
        }
        emitln("vpush {" + FloatRegIDToName(tmpreg) + "}");
        emitln("v" + op + ".f32 " + FloatRegIDToName(tmpreg) + ", " + FloatRegIDToName(op1reg) + ", " +
               FloatRegIDToName(op2reg));
        resreg = alloc_reg(tac->a_, tmpreg);
        if (tmpreg == resreg) {
          emitln("add sp, sp, #4");
        } else {
          emitln("vmov " + FloatRegIDToName(resreg) + ", " + FloatRegIDToName(tmpreg));
          emitln("vpop {" + FloatRegIDToName(tmpreg) + "}");
        }
      } else {
        emitln("v" + op + ".f32 " + FloatRegIDToName(resreg) + ", " + FloatRegIDToName(op1reg) + ", " +
               FloatRegIDToName(op2reg));
      }
    } else {
      int op1reg = alloc_reg(tac->b_);
      int op2reg = alloc_reg(tac->c_, op1reg);
      int resreg = alloc_reg(tac->a_, op2reg);
      if (resreg == op1reg || resreg == op2reg) {
        op1reg = alloc_reg(tac->b_);
        op2reg = alloc_reg(tac->c_, op1reg);
        int otherreg = glob_context_.USE_INT_REG_NUM;
        emitln(op + " " + IntRegIDToName(otherreg) + ", " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(op2reg));
        emitln("push {" + IntRegIDToName(otherreg) + "}");
        resreg = alloc_reg(tac->a_);
        emitln("pop {" + IntRegIDToName(resreg) + "}");
      } else {
        emitln(op + " " + IntRegIDToName(resreg) + ", " + IntRegIDToName(op1reg) + ", " + IntRegIDToName(op2reg));
      }
    }
  };

  auto int_divmod_operation = [&, this]() -> void {
    int op1reg = alloc_reg(tac->b_);
    int op2reg = alloc_reg(tac->c_, op1reg);
    emitln("push {r1-r3}");
    if (op1reg < op2reg) {
      emitln("push {" + IntRegIDToName(op1reg) + ", " + IntRegIDToName(op2reg) + "}");
    } else {
      emitln("push {" + IntRegIDToName(op2reg) + "}");
      emitln("push {" + IntRegIDToName(op1reg) + "}");
    }
    evit_int_reg(0);
    emitln("pop {r0, r1}");
    bool padding = false;
    {
      size_t stacksize = 3 * 4ULL + glob_context_.stack_size_for_args_ + glob_context_.stack_size_for_regsave_ +
                         glob_context_.stack_size_for_vars_;
      if (stacksize % 8 != 0) {
        padding = true;
        emitln("sub sp, sp, #4");
      }
    }
    if (tac->operation_ == TACOperationType::Div) {
      emitln("bl __aeabi_idiv");
    } else {
      emitln("bl __aeabi_idivmod");
    }
    if (padding) {
      emitln("add sp, sp, #4");
    }
    if (tac->operation_ == TACOperationType::Mod) {
      emitln("mov r0, r1");
    }
    emitln("pop {r1-r3}");
    int regid = alloc_reg(tac->a_, 0);
    emitln("mov " + IntRegIDToName(regid) + ", r0");
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
        addrreg = glob_context_.USE_INT_REG_NUM;
        emitln("add " + IntRegIDToName(addrreg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg) +
               ", LSL #2");
      }
      int valuereg = alloc_reg(tac->b_, addrreg);
      if (tac->b_->value_.Type() == SymbolValue::ValueType::Float) {
        emitln("vstr s" + std::to_string(valuereg) + ", [" + IntRegIDToName(addrreg) + "]");
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
        addrreg = glob_context_.USE_INT_REG_NUM;
        emitln("add " + IntRegIDToName(addrreg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg) +
               ", LSL #2");
      }
      int valuereg = alloc_reg(tac->a_, addrreg);
      if (tac->a_->value_.Type() == SymbolValue::ValueType::Float) {
        emitln("vldr " + FloatRegIDToName(valuereg) + ", [" + IntRegIDToName(addrreg) + "]");
      } else {
        emitln("ldr " + IntRegIDToName(valuereg) + ", [" + IntRegIDToName(addrreg) + "]");
      }
    } else {
      if (tac->b_->value_.UnderlyingType() == SymbolValue::ValueType::Float) {
        emitln("vmov.f32 " + FloatRegIDToName(dstreg) + ", " + FloatRegIDToName(valreg));
      } else {
        emitln("mov " + IntRegIDToName(dstreg) + ", " + IntRegIDToName(valreg));
      }
    }
  };

  auto do_call = [&, this]() -> void {
    //初始化一下
    glob_context_.stack_size_for_args_ = 0;
    //先压栈吧
    emitln("push {r1-r3}");
    emitln("vpush {s1-s15}");
    //全压！这是刚才压了的寄存器的大小
    int save_reg_size = 3 * 4 + 15 * 4;

    glob_context_.stack_size_for_args_ += save_reg_size;
    int nstackarg = 0;
    for (auto &record : glob_context_.arg_records_) {
      if (!record.storage_in_reg) {
        nstackarg++;
      }
    }
    //计算一下总大小
    size_t sumstack = 0;
    bool padding = false;
    sumstack += nstackarg * 4;
    sumstack += glob_context_.stack_size_for_args_;
    sumstack += glob_context_.stack_size_for_regsave_;
    sumstack += glob_context_.stack_size_for_vars_;
    //因为让栈8对齐，这里检查一下并对齐
    if (sumstack % 8 != 0) {
      padding = true;
      emitln("sub sp, sp, #4");
      glob_context_.stack_size_for_args_ += 4;
    }

    //先把所有栈上的压了，反向压栈。
    for (auto it = glob_context_.arg_records_.rbegin(); it != glob_context_.arg_records_.rend(); ++it) {
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
        int resreg = glob_context_.USE_INT_REG_NUM;
        emitln("add " + IntRegIDToName(resreg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg));
        emitln("push {" + IntRegIDToName(resreg) + "}");
        glob_context_.stack_size_for_args_ += 4;
      } else {
        int reg = alloc_reg(it->sym);
        if (it->sym->value_.Type() == SymbolValue::ValueType::Float) {
          emitln("vpush {" + FloatRegIDToName(reg) + "}");
          glob_context_.stack_size_for_args_ += 4;
        } else {
          emitln("push {" + IntRegIDToName(reg) + "}");
          glob_context_.stack_size_for_args_ += 4;
        }
      }
    }

    //然后处理寄存器
    //先让float的全压了，因为是倒序，保证了s0最后处理
    for (auto it = glob_context_.arg_records_.rbegin(); it != glob_context_.arg_records_.rend(); ++it) {
      if (!it->storage_in_reg) {
        continue;
      }
      if (it->sym->value_.Type() != SymbolValue::ValueType::Float) {
        continue;
      }
      int reg = alloc_reg(it->sym);
      if (reg != it->storage_pos) {
        evit_float_reg(it->storage_pos);
        emitln("vmov " + FloatRegIDToName(it->storage_pos) + ", " + FloatRegIDToName(reg));
      }
    }

    for (auto it = glob_context_.arg_records_.rbegin(); it != glob_context_.arg_records_.rend(); ++it) {
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
        int basereg = alloc_reg(basesym);
        int offreg = alloc_reg(offsym, basereg);
        int resreg = glob_context_.USE_INT_REG_NUM;
        emitln("add " + IntRegIDToName(resreg) + ", " + IntRegIDToName(basereg) + ", " + IntRegIDToName(offreg));
        evit_int_reg(it->storage_pos);
        emitln("mov " + IntRegIDToName(it->storage_pos) + ", " + IntRegIDToName(resreg));
      } else {
        int reg = alloc_reg(it->sym);
        if (reg != it->storage_pos) {
          evit_int_reg(it->storage_pos);
          emitln("mov " + IntRegIDToName(it->storage_pos) + ", " + IntRegIDToName(reg));
        }
      }
    }

    //可以call了
    emitln("bl " + tac->b_->get_tac_name(true));

    //去掉所有栈上arg
    uint32_t toaddstack = padding ? 4 : 0;
    toaddstack += nstackarg * 4;
    if (ArmHelper::IsImmediateValue(toaddstack)) {
      emitln("add sp, sp, #" + std::to_string(toaddstack));
    } else {
      auto splitaddstack = ArmHelper::DivideIntoImmediateValues(toaddstack);
      int otherreg = glob_context_.USE_INT_REG_NUM;
      emitln("ldr " + IntRegIDToName(otherreg) + ", =" + std::to_string(toaddstack));
      emitln("add sp, sp, " + IntRegIDToName(otherreg));
    }
    glob_context_.stack_size_for_args_ -= toaddstack;

    //恢复寄存器

    emitln("vpop {s1-s15}");
    emitln("pop {r1-r3}");

    glob_context_.stack_size_for_args_ -= save_reg_size;

    assert(glob_context_.stack_size_for_args_ == 0);

    //如果有接收变量
    if (tac->a_ != nullptr) {
      //返回在s0
      if (tac->a_->value_.Type() == SymbolValue::ValueType::Float) {
        //如果在栈上，我们直接认为s0处是它就可以了
        int regid = alloc_reg(tac->a_, 0);
        assert(regid != 0);
        emitln("vmov " + FloatRegIDToName(regid) + ", s0");
      } else {
        //同上
        int regid = alloc_reg(tac->a_, 0);
        assert(regid != 0);
        emitln("mov " + IntRegIDToName(regid) + ", r0");
      }
    }
    glob_context_.arg_nfloatregs_ = 0;
    glob_context_.arg_nintregs_ = 0;
    glob_context_.arg_stacksize_ = 0;
    glob_context_.arg_records_.clear();
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
    case TACOperationType::Undefined:
      throw std::runtime_error("Undefined operation");
      break;
    case TACOperationType::Add:
      simple_binary_operation("add");
      break;
    case TACOperationType::Sub:
      simple_binary_operation("sub");
      break;
    case TACOperationType::Mul:
      simple_binary_operation("mul");
      break;
    case TACOperationType::Div:
      if (tac->a_->value_.Type() == SymbolValue::ValueType::Float) {
        simple_binary_operation("div");
      } else {
        int_divmod_operation();
      }
      break;
    case TACOperationType::Mod:
      if (tac->a_->value_.Type() == SymbolValue::ValueType::Float) {
        simple_binary_operation("div");
        int resreg = alloc_reg(tac->a_);
        int op2reg = alloc_reg(tac->c_, resreg);
        emitln("vmul.f32 " + FloatRegIDToName(resreg) + ", " + FloatRegIDToName(resreg) + ", " +
               FloatRegIDToName(op2reg));
        int op1reg = alloc_reg(tac->b_, resreg);
        emitln("vsub.f32 " + FloatRegIDToName(resreg) + ", " + FloatRegIDToName(op1reg) + ", " +
               FloatRegIDToName(resreg));
      } else {
        int_divmod_operation();
      }
      break;
    case TACOperationType::UnaryPositive:
    case TACOperationType::Assign:
      assignment();
      break;
    case TACOperationType::UnaryMinus:
      if (tac->a_->value_.UnderlyingType() != tac->b_->value_.UnderlyingType()) {
        throw std::logic_error("Mismatch type in unary minus");
      } else {
        int valreg = alloc_reg(tac->b_);
        int dstreg = alloc_reg(tac->a_, valreg);
        //如果是浮点
        if (tac->b_->value_.UnderlyingType() == SymbolValue::ValueType::Float) {
          emitln("vneg.f32 s" + std::to_string(dstreg) + ", s" + std::to_string(valreg));
        } else {
          emitln("neg " + IntRegIDToName(dstreg) + ", " + IntRegIDToName(valreg));
        }
      }
      break;
    case TACOperationType::Argument:
    case TACOperationType::ArgumentAddress: {
      ArmUtil::GlobalContext::ArgRecord record;
      //浮点类型
      if (tac->operation_ == TACOperationType::Argument &&
          tac->a_->value_.UnderlyingType() == SymbolValue::ValueType::Float) {
        //如果小于16，可以用
        if (glob_context_.arg_nfloatregs_ < 16) {
          record.storage_in_reg = true;
          record.storage_pos = glob_context_.arg_nfloatregs_;
          glob_context_.arg_nfloatregs_++;
        } else {
          //否则存栈上
          record.storage_in_reg = false;
          record.storage_pos = glob_context_.arg_stacksize_;
          glob_context_.arg_stacksize_ += 4;
        }
      } else {
        if (glob_context_.arg_nintregs_ < 4) {
          record.storage_in_reg = true;
          record.storage_pos = glob_context_.arg_nintregs_;
          glob_context_.arg_nintregs_++;
        } else {
          //否则存栈上
          record.storage_in_reg = false;
          record.storage_pos = glob_context_.arg_stacksize_;
          glob_context_.arg_stacksize_ += 4;
        }
      }
      if (tac->operation_ == TACOperationType::ArgumentAddress) {
        record.isaddr = true;
      } else {
        record.isaddr = false;
      }
      record.sym = tac->a_;
      glob_context_.arg_records_.push_back(record);
      break;
    }
    case TACOperationType::Call:
      do_call();
      break;
    case TACOperationType::IntToFloat:
    case TACOperationType::FloatToInt:
      cast_operation();
      break;
    case TACOperationType::Label:
      break;
    default:
      throw std::logic_error("Unknown operation: " +
                             std::string(magic_enum::enum_name<TACOperationType>(tac->operation_)));
  }

  return ret;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler