#include <stdexcept>
#include "ASM/Optimizer.hh"
#include "TAC/TAC.hh"

namespace HaveFunCompiler {
namespace AssemblyBuilder {

using namespace ThreeAddressCode;

ConstantFoldingOptimizer::ConstantFoldingOptimizer(TACListPtr tacList, TACList::iterator fbegin, TACList::iterator fend)
    : fbegin_(fbegin), fend_(fend), tacls_(tacList) {}

int ConstantFoldingOptimizer::optimize() {
  auto it = fbegin_;
  int opt_count = 0;
  auto create_const = [](SymbolValue value) -> SymbolPtr {
    auto sym = std::make_shared<Symbol>();
    sym->type_ = SymbolType::Constant;
    sym->value_ = value;
    return sym;
  };
  // a = b;
  auto change2assign = [&opt_count](HaveFunCompiler::ThreeAddressCode::ThreeAddressCode &tac, SymbolPtr a,
                                    SymbolPtr b) -> void {
    tac.operation_ = TACOperationType::Assign;
    tac.a_ = a;
    tac.b_ = b;
    tac.c_ = nullptr;
    opt_count++;
  };
  auto is_binary_operation = [](TACOperationType type) -> bool {
    switch (type) {
      case TACOperationType::Add:
      case TACOperationType::Sub:
      case TACOperationType::Mul:
      case TACOperationType::Div:
      case TACOperationType::Mod:
      case TACOperationType::Equal:
      case TACOperationType::NotEqual:
      case TACOperationType::LessOrEqual:
      case TACOperationType::GreaterThan:
      case TACOperationType::GreaterOrEqual:
      case TACOperationType::LessThan:
        return true;
      default:
        return false;
    }
  };

  auto is_unary_operation = [](TACOperationType type) -> bool {
    switch (type) {
      case TACOperationType::UnaryMinus:
      case TACOperationType::UnaryNot:
      case TACOperationType::UnaryPositive:
      case TACOperationType::FloatToInt:
      case TACOperationType::IntToFloat:
        return true;
      default:
        return false;
    }
  };

  while (it != fend_) {
    auto &tac = *(*it);
    if (is_unary_operation(tac.operation_) && tac.b_->IsLiteral()) {
      switch (tac.operation_) {
        case TACOperationType::UnaryMinus: {
          change2assign(tac, tac.a_, create_const(-tac.b_->value_));
          break;
        };
        case TACOperationType::UnaryNot: {
          change2assign(tac, tac.a_, create_const(!tac.b_->value_));
          break;
        }
        case TACOperationType::UnaryPositive: {
          change2assign(tac, tac.a_, tac.b_);
          break;
        }
        case TACOperationType::IntToFloat: {
          if (tac.b_->value_.Type() == SymbolValue::ValueType::Float) {
            change2assign(tac, tac.a_, tac.b_);
          } else {
            change2assign(tac, tac.a_, create_const(static_cast<float>(tac.b_->value_.GetInt())));
          }
          break;
        }
        case TACOperationType::FloatToInt: {
          if (tac.b_->value_.Type() == SymbolValue::ValueType::Int) {
            change2assign(tac, tac.a_, tac.b_);
          } else {
            change2assign(tac, tac.a_, create_const(static_cast<int>(tac.b_->value_.GetFloat())));
          }
          break;
        }
        default:
          throw std::logic_error("ConstantFoldingOptimizer:Unreachable");
      }
    } else if (is_binary_operation(tac.operation_) && tac.b_->IsLiteral() && tac.c_->IsLiteral()) {
      switch (tac.operation_) {
        case TACOperationType::Add: {
          change2assign(tac, tac.a_, create_const(tac.b_->value_ + tac.c_->value_));
          break;
        }
        case TACOperationType::Sub: {
          change2assign(tac, tac.a_, create_const(tac.b_->value_ - tac.c_->value_));
          break;
        }
        case TACOperationType::Mul: {
          change2assign(tac, tac.a_, create_const(tac.b_->value_ * tac.c_->value_));
          break;
        }
        case TACOperationType::Div: {
          change2assign(tac, tac.a_, create_const(tac.b_->value_ / tac.c_->value_));
          break;
        }
        case TACOperationType::Mod: {
          change2assign(tac, tac.a_, create_const(tac.b_->value_ % tac.c_->value_));
          break;
        }
        case TACOperationType::Equal: {
          change2assign(tac, tac.a_, create_const(tac.b_->value_ == tac.c_->value_));
          break;
        }
        case TACOperationType::NotEqual: {
          change2assign(tac, tac.a_, create_const(tac.b_->value_ != tac.c_->value_));
          break;
        }
        case TACOperationType::GreaterOrEqual: {
          change2assign(tac, tac.a_, create_const(tac.b_->value_ >= tac.c_->value_));
          break;
        }
        case TACOperationType::LessThan: {
          change2assign(tac, tac.a_, create_const(tac.b_->value_ < tac.c_->value_));
          break;
        }
        case TACOperationType::LessOrEqual: {
          change2assign(tac, tac.a_, create_const(tac.b_->value_ <= tac.c_->value_));
          break;
        }
        case TACOperationType::GreaterThan: {
          change2assign(tac, tac.a_, create_const(tac.b_->value_ > tac.c_->value_));
          break;
        }

        default:
          throw std::logic_error("ConstantFoldingOptimizer:Unreachable");
      }
    }
    ++it;
  }
  return opt_count;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler