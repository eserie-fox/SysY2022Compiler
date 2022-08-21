#include "ASM/arm/ArmBuilder.hh"
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/Optimizer.hh"
#include "ASM/arm/ArmHelper.hh"
#include "ASM/arm/ArmPostOptimizer.hh"
#include "ASM/arm/RegAllocator.hh"
#include "MacroUtil.hh"
#include "MagicEnum.hh"
#include "TAC/TAC.hh"

extern int OP_flag;
extern bool tac_only;

namespace HaveFunCompiler {
namespace AssemblyBuilder {
using namespace ThreeAddressCode;
ArmBuilder::ArmBuilder(TACListPtr tac_list) : data_pool_distance_(0), data_pool_id_(0), tac_list_(tac_list),counter(0) {}

bool ArmBuilder::AppendPrefix() {
  std::string *pfunc_section;
  auto emitln = [&pfunc_section](const std::string &inst) -> void {
    pfunc_section->append(inst);
    pfunc_section->append("\n");
  };
  func_sections_.emplace_back("_builtin_clear");
  pfunc_section = &func_sections_.back().body_;
  emitln(".text");
  emitln(".align 4");
  // emitln(".global _builtin_clear");
  emitln("_builtin_clear:");
  emitln("mov r2, #0");
  emitln("cmp r1, #0");
  emitln("ble _builtin_clear_break");
  emitln("_builtin_clear_loop:");
  emitln("str r2, [r0], #+4");
  emitln("subs r1, #1");
  emitln("bgt _builtin_clear_loop");
  emitln("_builtin_clear_break:");
  emitln("bx lr");

  auto forward_declare = [&, this](std::string func_name) -> void {
    func_sections_.emplace_back(func_name);
    pfunc_section = &func_sections_.back().body_;
    emitln(".text");
    emitln(".global " + func_name);
  };
  forward_declare("getint");
  forward_declare("getch");
  forward_declare("getfloat");
  forward_declare("getarray");
  forward_declare("getfarray");
  forward_declare("putint");
  forward_declare("putch");
  forward_declare("putfloat");
  forward_declare("putarray");
  forward_declare("putfarray");
  forward_declare("putf");
  forward_declare("_sysy_starttime");
  forward_declare("_sysy_stoptime");
  forward_declare("__aeabi_idiv");
  forward_declare("__aeabi_idivmod");
  return true;
}

bool ArmBuilder::AppendSuffix() { return true; }

bool ArmBuilder::TranslateGlobal() {
  ArmUtil::GlobalContextGuard glob_context_guard(glob_context_);
  current_ = tac_list_->begin();
  end_ = tac_list_->end();
  int func_level = 0;

  if (tac_only)
  {
    for (; current_ != end_; ++current_) {
      auto tac = *current_;
      switch (tac->operation_) {
        case TACOperationType::FunctionBegin:
          func_level++;
          break;
        case TACOperationType::FunctionEnd:
          func_level--;
          break;
        default:
          if (!func_level) {
            target_output_->append(tac->ToString());
            target_output_->append("\n");
          }
        break;
      }
    }
    return true;
  }

  //第一遍进行变量地址分配
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
        case TACOperationType::Constant:
          if (!func_level) {
            if (!tac->a_->IsGlobal()) {
              throw std::logic_error("Not global variable in global region: " + tac->a_->get_name());
            }
            if (tac->a_->type_ != SymbolType::Constant) {
              throw std::runtime_error("Expected constant in const declaration, but actually " +
                                       std::string(magic_enum::enum_name<SymbolType>(tac->a_->type_)));
            }
            if (tac->a_->value_.Type() != SymbolValue::ValueType::Array) {
              throw std::runtime_error(
                  "Only constant array allowed in const declaration, but actually " +
                  std::string(magic_enum::enum_name<SymbolValue::ValueType>(tac->a_->value_.Type())));
            }

            //字面量跳过
            if (tac->a_->IsLiteral()) {
              break;
            }

            //一定没有全局临时常量
            if (tac->a_->IsGlobalTemp()) {
              throw std::logic_error("Illegal global temporary constant");
            } else if (tac->a_->name_.value_or("").length() > 3 && tac->a_->name_.value()[2] == 'U') {
              //对于非临时变量，进行存储声明
              data_section_ += DeclareDataToASMString(tac);
              AddDataRef(tac);
            }
          }
          break;
        case TACOperationType::Variable:
          if (!func_level) {
            if (!tac->a_->IsGlobal()) {
              throw std::logic_error("Not global variable in global region: " + tac->a_->get_name());
            }
            if (tac->a_->type_ != SymbolType::Variable) {
              throw std::runtime_error("Expected variable in var declaration, but actually " +
                                       std::string(magic_enum::enum_name<SymbolType>(tac->a_->type_)));
            }
            if (!tac->a_->value_.IsNumericType() && tac->a_->value_.Type() != SymbolValue::ValueType::Array) {
              throw std::runtime_error(
                  "Expected array or int/float in var declaration, but actually " +
                  std::string(magic_enum::enum_name<SymbolValue::ValueType>(tac->a_->value_.Type())));
            }

            //字面量跳过
            if (tac->a_->IsLiteral()) {
              break;
            }

            //进行栈空间分配
            if (tac->a_->IsGlobalTemp()) {
              assert(!glob_context_.var_stack_pos.count(tac->a_));
              glob_context_.var_stack_pos[tac->a_] = glob_context_.stack_size_for_vars_;
              glob_context_.stack_size_for_vars_ += 4;
            } else if (tac->a_->name_.value_or("").length() > 3 && tac->a_->name_.value()[2] == 'U') {
              data_section_ += DeclareDataToASMString(tac);
              AddDataRef(tac);
            }
          }
          break;
        default:
          break;
      }
    }
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return false;
  }

  current_ = tac_list_->begin();
  end_ = tac_list_->end();
  assert(func_level == 0);

  //添加一个新函数在列表
  func_sections_.emplace_back("main");
  //绑定到body，后面简写
  auto *pfunc_section = &func_sections_.back().body_;
  auto emit = [pfunc_section, this](const std::string &inst) -> void { (*pfunc_section) += inst; };
  auto emitln = [pfunc_section, this](const std::string &inst) -> void {
    pfunc_section->append(inst);
    pfunc_section->append("\n");
    data_pool_distance_ += ArmHelper::CountLines(inst) + 1;
    if (data_pool_distance_ > DATA_POOL_DISTANCE_THRESHOLD) {
      (*pfunc_section) += EndCurrentDataPool();
    }
  };

  emitln(".text");
  emitln(".align 4");
  emitln(".global main");
  emitln("main:");

  emitln("push {r4-r12, lr}");
  emitln("vpush {s16-s31}");
  glob_context_.stack_size_for_regsave_ = 10 * 4 + 16 * 4;
  if (!ArmHelper::EmitImmediateInstWithCheck(emitln, "sub", "sp", "sp", glob_context_.stack_size_for_vars_)) {
    emitln("ldr ip, =" + std::to_string(glob_context_.stack_size_for_vars_));
    emitln("sub sp, sp, ip");
  }

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
        case TACOperationType::Constant:
          break;
        case TACOperationType::Variable:
          break;
        default:
          if (!func_level) {
            emit(GlobalTACToASMString(tac));
          }
          break;
      }
    }
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return false;
  }
  if (!ArmHelper::EmitImmediateInstWithCheck(emitln, "add", "sp", "sp", glob_context_.stack_size_for_vars_)) {
    emitln("ldr ip, =" + std::to_string(glob_context_.stack_size_for_vars_));
    emitln("add sp, sp, ip");
  }
  glob_context_.stack_size_for_vars_ = 0;
  emitln("vpop {s16-s31}");
  emitln("pop {r4-r12 ,lr}");
  glob_context_.stack_size_for_regsave_ = 0;
  emitln("push {lr}");
  emitln("sub sp, sp, #4");
  emitln("bl S0U_main");
  emitln("add sp, sp, #4");
  emitln("pop {lr}");
  emitln("bx lr");

  return true;
}

bool ArmBuilder::TranslateFunction() {
  //[current_,end_)区间内为即将处理的函数
  //拿到func_context_guard确保func_context拥有正确初始化和析构行为
  auto func_context_guard = ArmUtil::FunctionContextGuard(func_context_);
  {
    if (OP_flag)
    {
      std::shared_ptr<OptimizeController> opController = std::make_shared<OptimizeController_Simple>(tac_list_, current_, end_);
      opController->doOptimize();
    }

    if (tac_only)
    {
      target_output_->append("\n");
      for (auto it = current_; it != end_; ++it)
      {
        target_output_->append((*it)->ToString());
        target_output_->append("\n");
      }
      return true;
    }

    // 生成控制流图
    auto cfg = std::make_shared<ControlFlowGraph>(current_, end_);

    // 移除不可达代码
    auto &deadCode = cfg->get_unreachableTACItrList();
    for (auto it : deadCode) tac_list_->erase(it);

    // 解析寄存器分配
    func_context_.reg_alloc_ = new RegAllocator(LiveIntervalAnalyzer(cfg));
  }
  //开头label包含了函数名
  auto func_label_itr = current_;
  ++func_label_itr;
  auto func_label = (*func_label_itr)->a_;
  std::string func_name = func_label->get_name();
  //添加一个新函数在列表
  func_sections_.emplace_back(func_name);
  //绑定到body，后面简写
  auto *pfunc_section = &func_sections_.back().body_;
  auto emit = [pfunc_section, this](const std::string &inst) -> void { (*pfunc_section) += inst; };
  auto emitln = [pfunc_section, this](const std::string &inst) -> void {
    pfunc_section->append(inst);
    pfunc_section->append("\n");
    data_pool_distance_ += ArmHelper::CountLines(inst) + 1;
    if (data_pool_distance_ > DATA_POOL_DISTANCE_THRESHOLD) {
      (*pfunc_section) += EndCurrentDataPool();
    }
  };
  //添加函数头
  emitln(".text");
  emitln(".align 4");
  emitln(".global " + func_name);
  emitln(func_name + ":");
  //将要用到的寄存器保存起来
  func_context_.func_attr_ = func_context_.reg_alloc_->get_SymAttribute(func_label);
  //保存了的寄存器列表
  std::vector<uint32_t> &saveintregs = func_context_.saveintregs_;
  std::vector<std::pair<uint32_t, uint32_t>> &savefloatregs = func_context_.savefloatregs_;
  auto push_reg = [](std::vector<std::pair<uint32_t, uint32_t>> &reg_list, uint32_t newreg) -> void {
    if (reg_list.empty()) {
      reg_list.emplace_back(newreg, newreg);
      return;
    }
    if (reg_list.back().second == newreg - 1 && (newreg - reg_list.back().first + 1) <= 16) {
      reg_list.back().second = newreg;
      return;
    }
    reg_list.emplace_back(newreg, newreg);
  };

  //保存会修改的通用寄存器
  for (int i = 4; i < 13; i++) {
    //如果第i号通用寄存器要用
    if (ISSET_UINT(func_context_.func_attr_.attr.used_regs.intRegs, i)) {
      //那么保存它
      saveintregs.push_back(i);
      func_context_.stack_size_for_regsave_ += 4;
    }
  }

  //如果lr会被用到单独保存一下
  if (ISSET_UINT(func_context_.func_attr_.attr.used_regs.intRegs, LR_REGID)) {
    saveintregs.push_back(LR_REGID);
    func_context_.stack_size_for_regsave_ += 4;
  }

  //保存刚才确定好的通用寄存器
  if (!saveintregs.empty()) {
    std::string inst = "push {";
    for (size_t i = 0; i < saveintregs.size(); i++) {
      inst += IntRegIDToName(saveintregs[i]);
      if (i + 1 < saveintregs.size()) {
        inst += ", ";
      }
    }
    inst += "}";
    emitln(inst);
  }
  //保存浮点寄存器
  for (int i = 16; i < 32; i++) {
    if (ISSET_UINT(func_context_.func_attr_.attr.used_regs.floatRegs, i)) {
      push_reg(savefloatregs, i);
      func_context_.stack_size_for_regsave_ += 4;
    }
  }
  //保存刚才确定好的浮点寄存器
  {
    for (auto regid : savefloatregs) {
      if (regid.first == regid.second) {
        emitln("vpush { " + FloatRegIDToName(regid.first) + " }");
      } else {
        emitln("vpush { " + FloatRegIDToName(regid.first) + "-" + FloatRegIDToName(regid.second) + " }");
      }
    }
  }
  //为变量分配栈空间
  func_context_.stack_size_for_vars_ = func_context_.func_attr_.value;
  //可能用很大的栈空间，立即数存不下，保险起见Divide一下
  auto &var_stack_immvals = func_context_.var_stack_immvals_;
  var_stack_immvals = ArmHelper::DivideIntoImmediateValues(func_context_.stack_size_for_vars_);
  size_t test_varsize_imm = 0;
  for (auto immval : var_stack_immvals) {
    test_varsize_imm += immval;
    bool check = ArmHelper::EmitImmediateInstWithCheck(emitln, "sub", "sp", "sp", immval);
    assert(check);
  }
  assert((int)test_varsize_imm == func_context_.stack_size_for_vars_);

  //为后面栈的释放我们反向一下。
  std::reverse(var_stack_immvals.begin(), var_stack_immvals.end());
  std::reverse(savefloatregs.begin(), savefloatregs.end());
  // std::reverse(saveintregs.begin(), saveintregs.end());

  //函数体翻译
  //跳过label和fbegin
  assert((*current_)->operation_ == TACOperationType::FunctionBegin);
  ++current_;
  assert((*current_)->operation_ == TACOperationType::Label);
  ++current_;
  //去掉fend
  --end_;
  assert((*end_)->operation_ == TACOperationType::FunctionEnd);
  auto save_end_ = **end_;
  // 尾部强行加一个return，安全起见。
  (*end_)->operation_ = TACOperationType::Return;
  (*end_)->a_ = nullptr;
  (*end_)->b_ = nullptr;
  (*end_)->c_ = nullptr;
  ++end_;

  auto can_retcall_optimize = [&, this]() -> bool {
    // bool has_stack_arg = false;
    for (const auto &record : func_context_.arg_records_) {
      // if (!record.storage_in_reg) {
      //   has_stack_arg = true;
      // }
      if (record.sym->value_.Type() == SymbolValue::ValueType::Array) {
        //非形参的局部数组会阻碍retcall优化
        if (!record.sym->IsGlobal() &&
            record.sym->value_.GetArrayDescriptor()->base_addr.lock()->offset_ != Symbol::PARAM) {
          return false;
        }
      }
    }
    //这一部分判定移动到ArmBuilderFuncTAC了。
    // //如果有溢出到栈的arg，那么就需要看一下栈空间够不够挪动。
    // if (has_stack_arg) {
    //   int count = func_context_.saveintregs_.size() + func_context_.savefloatregs_.size();
    //   if (count < 4) {
    //     return false;
    //   }
    // }

    return true;
  };

  for (; current_ != end_; ++current_) {
    if ((*current_)->operation_ == TACOperationType::Call) {
      auto next = current_;
      ++next;
      if (next != end_ && (*next)->operation_ == TACOperationType::Return) {
        if ((*current_)->a_ == (*next)->a_ && can_retcall_optimize()) {
          TACPtr taccallret = std::make_shared<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>();
          taccallret->operation_ = TACOperationType::CallAndReturn;
          taccallret->b_ = (*current_)->b_;
          emit(FuncTACToASMString(taccallret));
          current_ = next;
          continue;
        }
      }
    }
    emit(FuncTACToASMString(*current_));
  }
  --end_;
  **end_ = save_end_;
  //还原fend
  ++end_;

  //为了安全起见，强行加一个return
  // TACPtr tacret = std::make_shared<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>();
  // tacret->operation_ = TACOperationType::Return;
  // emit(FuncTACToASMString(tacret));

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
    if ((*it)->operation_ == TACOperationType::FunctionBegin) {
      // label 后面接funcbegin标志着函数开始
      auto prev_it = it++;
      if (it != end_ && (*it)->operation_ == TACOperationType::Label) {
        func_level++;
        assert(func_level < 2);
        //是函数，我们储存其开头位置
        func_begin = prev_it;
      } else {
        //不满足条件的话是错误
        throw std::runtime_error("Unexpected " +
                                 std::string(magic_enum::enum_name<TACOperationType>((*it)->operation_)) +
                                 " after function begin");
      }
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
  data_section_.clear();
  ref_data_.clear();
  func_sections_.clear();

  if (!tac_only) {
    if (!AppendPrefix()) {
      return false;
    }
  }
  if (!TranslateGlobal()) {
    return false;
  }
  target_output_->append(data_section_);
  if (!TranslateFunctions()) {
    return false;
  }
  if (!tac_only) {
    for (auto &func_section : func_sections_) {
      target_output_->append(func_section.body_);
    }
    target_output_->append(EndCurrentDataPool(true));
    if (!AppendSuffix()) {
      return false;
    }
    if (OP_flag) {
      *target_output_ = ArmPostOptimizer(*target_output_).optimize();
    }
  }

  return true;
}

std::string ArmBuilder::GetVariableName(SymbolPtr sym) {
  if (sym->value_.Type() == SymbolValue::ValueType::Array) {
    return sym->value_.GetArrayDescriptor()->base_addr.lock()->get_tac_name(true);
  }
  return sym->get_tac_name(true);
}

std::string ArmBuilder::ToDataRefName(std::string name) { return "_ref_" + name + "_" + std::to_string(data_pool_id_); }

std::string ArmBuilder::DeclareDataToASMString(TACPtr tac) {
  auto sym = tac->a_;
  //注释和align设置
  std::string ret = "// " + tac->ToString() + "\n.bss\n.align 4\n";
  //数据label
  ret += GetVariableName(sym) + ":\n";
  //如果是普通的int或float都是1单位sz，数组则可能多个
  size_t sz = 1;
  //数组声明
  if (sym->value_.Type() == SymbolValue::ValueType::Array) {
    sz = sym->value_.GetArrayDescriptor()->dimensions[0];
  }
  ret += ".skip " + std::to_string(sz * 4) + "\n";
  return ret;
}

void ArmBuilder::AddDataRef(TACPtr tac) {
  std::string name = GetVariableName(tac->a_);
  ref_data_.push_back(name);
  // std::string ret = "// add reference to data '" + name + "'\n";
  // ret += ToDataRefName(name) + ": .word " + name + "\n";
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
  throw std::runtime_error("Unexpected int regid " + std::to_string(regid));
}

std::string ArmBuilder::FloatRegIDToName(int regid) {
  if (0 <= regid && regid < 32) {
    return std::string("s") + std::to_string(regid);
  }
  throw std::runtime_error("Unexpected float regid " + std::to_string(regid));
}

std::string ArmBuilder::EndCurrentDataPool(bool ignorebranch) {
  data_pool_distance_ = 0;
  std::string ret;
  auto emitln = [&ret](const std::string str) -> void {
    ret.append(str);
    ret.append("\n");
  };
  if (!ignorebranch) {
    emitln("b _ignore_data_pool_" + std::to_string(data_pool_id_));
  }
  emitln(".align 4");
  emitln(".ltorg");
  if (!ref_data_.empty()) {
    emitln(".align 4");
  }
  for (const auto &name : ref_data_) {
    emitln("// add reference to data '" + name);
    emitln(ToDataRefName(name) + ": .word " + name);
  }
  if (!ignorebranch) {
    emitln(".align 4");
    emitln("_ignore_data_pool_" + std::to_string(data_pool_id_) + ":");
  }
  data_pool_id_++;
  return ret;
}
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler