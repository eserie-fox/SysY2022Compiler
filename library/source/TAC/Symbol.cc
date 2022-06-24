#include "TAC/Symbol.hh"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include "Exceptions.hh"
#include "MagicEnum.hh"
#include "TAC/Expression.hh"

namespace HaveFunCompiler {
namespace ThreeAddressCode {

const std::unordered_map<SymbolValue::ValueType, size_t> ValueTypeSize = {{SymbolValue::ValueType::Float, 4},
                                                                          {SymbolValue::ValueType::Int, 4}};

SymbolValue::SymbolValue() { type = ValueType::Void; }

SymbolValue::SymbolValue(const SymbolValue &other) : type(other.type), value(other.value) {}

SymbolValue::SymbolValue(float v) : type(ValueType::Float), value(v) {}
SymbolValue::SymbolValue(int v) : type(ValueType::Int), value(v) {}
SymbolValue::SymbolValue(const char *v) : type(ValueType::Str), value(v) {}
SymbolValue::SymbolValue(std::shared_ptr<ParameterList> v) : type(ValueType::Parameters), value(v) {}
SymbolValue::SymbolValue(std::shared_ptr<ArrayDescriptor> v) : type(ValueType::Array), value(v) {}
float SymbolValue::GetFloat() const {
  if (type != ValueType::Float) {
    if (type == ValueType::Array) {
      if (auto ad = GetArrayDescriptor(); ad->value_type == ValueType::Float) {
        if (ad->subarray->size() == 1) {
          if (auto it = ad->subarray->find(0); it != ad->subarray->end()) {
            return it->second->ret->value_.GetFloat();
          }
        }
      }
    }
    throw std::runtime_error("SymbolValue::GetFloat (Actually " + std::string(magic_enum::enum_name<ValueType>(type)) +
                             ")");
  }
  return std::get<float>(value);
}
int SymbolValue::GetInt() const {
  if (type != ValueType::Int) {
    if (type == ValueType::Array) {
      if (auto ad = GetArrayDescriptor(); ad->value_type == ValueType::Int) {
        if (ad->subarray->size() == 1) {
          if (auto it = ad->subarray->find(0); it != ad->subarray->end()) {
            return it->second->ret->value_.GetInt();
          }
        }
      }
    }
    throw std::runtime_error("Type mismatch in SymbolValue::GetInt (Actually " +
                             std::string(magic_enum::enum_name<ValueType>(type)) + ")");
  }
  return std::get<int>(value);
}
const char *SymbolValue::GetStr() const {
  if (type != ValueType::Str) {
    throw std::runtime_error("Type mismatch in SymbolValue::GetStr (Actually " +
                             std::string(magic_enum::enum_name<ValueType>(type)) + ")");
  }
  return std::get<const char *>(value);
}
std::shared_ptr<ParameterList> SymbolValue::GetParameters() const {
  if (type != ValueType::Parameters) {
    throw std::runtime_error("Type mismatch in SymbolValue::GetParameters (Actually " +
                             std::string(magic_enum::enum_name<ValueType>(type)) + ")");
  }
  return std::get<std::shared_ptr<ParameterList>>(value);
}
std::shared_ptr<ArrayDescriptor> SymbolValue::GetArrayDescriptor() const {
  if (type != ValueType::Array) {
    throw std::runtime_error("Type mismatch in SymbolValue::GetArrayDescriptor (Actually " +
                             std::string(magic_enum::enum_name<ValueType>(type)) + ")");
  }
  return std::get<std::shared_ptr<ArrayDescriptor>>(value);
}

std::string SymbolValue::ToString() const {
  switch (type) {
    case ValueType::Int:
      return std::to_string(GetInt());
    case ValueType::Float:
      return std::to_string(GetFloat());
    case ValueType::Str:
      return std::string(GetStr());
    case ValueType::Void:
      return "ERROR";
    case ValueType::Array: {
      auto array = GetArrayDescriptor();
      std::string name = "nullname";
      // if (!array->base_addr.expired())
      name = array->base_addr.lock()->name_.value_or("nullname");
      if (array->base_offset->type_ == SymbolType::Constant) {
        name += "[" + array->base_offset->value_.ToString() + "]";
      }
      // else {
      //   name += "[" + array->base_offset->name_.value() + "]";
      // }
      return name;
    }
    default:
      break;
  }
  return "notset";
}

SymbolValue &SymbolValue::operator=(const SymbolValue &other) {
  type = other.type;
  value = other.value;
  return *this;
}

bool SymbolValue::IsNumericType() const {
  if (type == ValueType::Float || type == ValueType::Int) {
    return true;
  }
  return false;
}
SymbolValue SymbolValue::operator+(const SymbolValue &other) const {
  if (!this->IsNumericType() || !other.IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type) + ", " +
                             std::to_string((int)other.type));
  }
  if (type == ValueType::Float || other.type == ValueType::Float) {
    float res;
    if (type == ValueType::Float) {
      res = GetFloat();
    } else {
      res = static_cast<float>(GetInt());
    }
    if (other.type == ValueType::Float) {
      res += other.GetFloat();
    } else {
      res += static_cast<float>(other.GetInt());
    }
    return SymbolValue(res);
  }
  return SymbolValue(GetInt() + other.GetInt());
}
SymbolValue SymbolValue::operator-(const SymbolValue &other) const {
  if (!IsNumericType() || !other.IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type) + ", " +
                             std::to_string((int)other.type));
  }
  if (type == ValueType::Float || other.type == ValueType::Float) {
    float res;
    if (type == ValueType::Float) {
      res = GetFloat();
    } else {
      res = static_cast<float>(GetInt());
    }
    if (other.type == ValueType::Float) {
      res -= other.GetFloat();
    } else {
      res -= static_cast<float>(other.GetInt());
    }
    return SymbolValue(res);
  }
  return SymbolValue(GetInt() - other.GetInt());
}
SymbolValue SymbolValue::operator*(const SymbolValue &other) const {
  if (!IsNumericType() || !other.IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type) + ", " +
                             std::to_string((int)other.type));
  }
  if (type == ValueType::Float || other.type == ValueType::Float) {
    float res;
    if (type == ValueType::Float) {
      res = GetFloat();
    } else {
      res = static_cast<float>(GetInt());
    }
    if (other.type == ValueType::Float) {
      res *= other.GetFloat();
    } else {
      res *= static_cast<float>(other.GetInt());
    }
    return SymbolValue(res);
  }
  return SymbolValue(GetInt() * other.GetInt());
}
SymbolValue SymbolValue::operator/(const SymbolValue &other) const {
  if (!IsNumericType() || !other.IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type) + ", " +
                             std::to_string((int)other.type));
  }
  if (type == ValueType::Float || other.type == ValueType::Float) {
    float res;
    if (type == ValueType::Float) {
      res = GetFloat();
    } else {
      res = static_cast<float>(GetInt());
    }
    if (other.type == ValueType::Float) {
      res /= other.GetFloat();
    } else {
      res /= static_cast<float>(other.GetInt());
    }
    return SymbolValue(res);
  }
  return SymbolValue(GetInt() / other.GetInt());
}
SymbolValue SymbolValue::operator%(const SymbolValue &other) const {
  if (!IsNumericType() || !other.IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type) + ", " +
                             std::to_string((int)other.type));
  }
  if (type == ValueType::Float || other.type == ValueType::Float) {
    float res;
    if (type == ValueType::Float) {
      res = GetFloat();
    } else {
      res = static_cast<float>(GetInt());
    }
    if (other.type == ValueType::Float) {
      res = std::fmod(res, other.GetFloat());
    } else {
      res = std::fmod(res, static_cast<double>(other.GetInt()));
    }
    return SymbolValue(res);
  }
  return SymbolValue(GetInt() % other.GetInt());
}
SymbolValue SymbolValue::operator!() const { return SymbolValue((int)(!this->operator bool())); }
SymbolValue SymbolValue::operator+() const {
  if (!IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type));
  }
  return *this;
}
SymbolValue SymbolValue::operator-() const {
  if (!IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type));
  }
  if (type == ValueType::Float) {
    float res;
    res = GetFloat();
    return SymbolValue(-res);
  } else {
    int res = GetInt();
    return SymbolValue(-res);
  }
}
SymbolValue SymbolValue::operator<(const SymbolValue &other) const {
  if (!IsNumericType() || !other.IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type) + ", " +
                             std::to_string((int)other.type));
  }
  if (type == ValueType::Float) {
    float ThisVal;
    ThisVal = GetFloat();
    if (other.type == ValueType::Float) {
      float OtherVal;
      OtherVal = other.GetFloat();
      return SymbolValue((int)(ThisVal < OtherVal));
    } else {
      int OtherVal = other.GetInt();
      return SymbolValue((int)(ThisVal < OtherVal));
    }
  } else {
    int ThisVal = GetInt();
    if (other.type == ValueType::Float) {
      float OtherVal;
      OtherVal = other.GetFloat();
      return SymbolValue((int)(ThisVal < OtherVal));
    } else {
      int OtherVal = other.GetInt();
      return SymbolValue((int)(ThisVal < OtherVal));
    }
  }
}
SymbolValue SymbolValue::operator>(const SymbolValue &other) const {
  if (!IsNumericType() || !other.IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type) + ", " +
                             std::to_string((int)other.type));
  }
  return other < *this;
}
SymbolValue SymbolValue::operator<=(const SymbolValue &other) const {
  if (!IsNumericType() || !other.IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type) + ", " +
                             std::to_string((int)other.type));
  }
  return !(other < *this);
}
SymbolValue SymbolValue::operator>=(const SymbolValue &other) const {
  if (!IsNumericType() || !other.IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type) + ", " +
                             std::to_string((int)other.type));
  }
  return !(*this < other);
}
SymbolValue SymbolValue::operator==(const SymbolValue &other) const {
  if (!IsNumericType() || !other.IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type) + ", " +
                             std::to_string((int)other.type));
  }
  if (type == ValueType::Float) {
    float ThisVal;
    ThisVal = GetFloat();
    if (other.type == ValueType::Float) {
      float OtherVal;
      OtherVal = other.GetFloat();
      return SymbolValue((int)(ThisVal == OtherVal));
    } else {
      int OtherVal = other.GetInt();
      return SymbolValue((int)(ThisVal == OtherVal));
    }
  } else {
    int ThisVal = GetInt();
    if (other.type == ValueType::Float) {
      float OtherVal;
      OtherVal = other.GetFloat();
      return SymbolValue((int)(ThisVal == OtherVal));
    } else {
      int OtherVal = other.GetInt();
      return SymbolValue((int)(ThisVal == OtherVal));
    }
  }
}
SymbolValue SymbolValue::operator!=(const SymbolValue &other) const { return !(*this == other); }
SymbolValue SymbolValue::operator&&(const SymbolValue &other) const {
  if (!IsNumericType() || !other.IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type) + ", " +
                             std::to_string((int)other.type));
  }
  if (type == ValueType::Float) {
    float ThisVal;
    ThisVal = GetFloat();
    if (other.type == ValueType::Float) {
      float OtherVal;
      OtherVal = other.GetFloat();
      return SymbolValue((int)(ThisVal && OtherVal));
    } else {
      int OtherVal = other.GetInt();
      return SymbolValue((int)(ThisVal && OtherVal));
    }
  } else {
    int ThisVal = GetInt();
    if (other.type == ValueType::Float) {
      float OtherVal;
      OtherVal = other.GetFloat();
      return SymbolValue((int)(ThisVal && OtherVal));
    } else {
      int OtherVal = other.GetInt();
      return SymbolValue((int)(ThisVal && OtherVal));
    }
  }
}
SymbolValue SymbolValue::operator||(const SymbolValue &other) const {
  if (!IsNumericType() || !other.IsNumericType()) {
    throw std::runtime_error("Not numeric type : " + std::to_string((int)type) + ", " +
                             std::to_string((int)other.type));
  }
  if (type == ValueType::Float) {
    float ThisVal;
    ThisVal = GetFloat();
    if (other.type == ValueType::Float) {
      float OtherVal;
      OtherVal = other.GetFloat();
      return SymbolValue((int)(ThisVal || OtherVal));
    } else {
      int OtherVal = other.GetInt();
      return SymbolValue((int)(ThisVal || OtherVal));
    }
  } else {
    int ThisVal = GetInt();
    if (other.type == ValueType::Float) {
      float OtherVal;
      OtherVal = other.GetFloat();
      return SymbolValue((int)(ThisVal || OtherVal));
    } else {
      int OtherVal = other.GetInt();
      return SymbolValue((int)(ThisVal || OtherVal));
    }
  }
}

bool SymbolValue::IsAssignableTo(const SymbolValue &other) const {
  bool isArrayMe = Type() == ValueType::Array;
  bool isArrayOther = other.Type() == ValueType::Array;
  if (isArrayMe || isArrayOther) {
    if (isArrayMe && isArrayOther) {
      auto myAD = GetArrayDescriptor();
      auto otherAD = other.GetArrayDescriptor();
      if (myAD->value_type != otherAD->value_type) {
        return false;
      }
      if (myAD->dimensions.size() != otherAD->dimensions.size()) {
        return false;
      }
      for (size_t i = 1; i < myAD->dimensions.size(); i++) {
        if (myAD->dimensions[i] != otherAD->dimensions[i]) {
          return false;
        }
      }
      return true;
    }
    if (isArrayMe) {
      auto myAD = GetArrayDescriptor();
      if (!myAD->dimensions.empty()) {
        return false;
      }
      if (myAD->value_type != other.Type()) {
        return false;
      }
      return true;
    } else {
      auto otherAD = other.GetArrayDescriptor();
      if (!otherAD->dimensions.empty()) {
        return false;
      }
      if (otherAD->value_type != Type()) {
        return false;
      }
      return true;
    }
  }
  if (!IsNumericType() || !other.IsNumericType()) {
    return false;
  }

  return (other.Type() == Type());
}

std::string SymbolValue::TypeToString() const {
  std::string ret = std::string(magic_enum::enum_name<ValueType>(Type()));

  switch (Type()) {
    case ValueType::Array: {
      ret = std::string(magic_enum::enum_name<ValueType>(GetArrayDescriptor()->value_type));
      auto arrayDescriptor = GetArrayDescriptor();
      for (size_t i = 0; i < arrayDescriptor->dimensions.size(); i++) {
        ret += "[";
        if (i || arrayDescriptor->dimensions[i]) {
          ret += std::to_string(arrayDescriptor->dimensions[i]);
        }
        ret += "]";
      }
      break;
    }
    case ValueType::Parameters: {
      ret += ": ";
      auto param = GetParameters();
      ret += std::string(magic_enum::enum_name<ValueType>(param->get_return_type()));
      ret += "(";
      for (auto type : *param) {
        ret += type->value_.TypeToString() + ",";
      }
      if (param->empty()) {
        ret.push_back(')');
      } else {
        ret.back() = ')';
      }
    }
    default: {
      break;
    }
  }

  return ret;
}

bool SymbolValue::HasOperatablity() {
  if (Type() == ValueType::Array) {
    auto ad = GetArrayDescriptor();
    if (!ad->dimensions.empty()) {
      return false;
    }
    return true;
  }
  if (Type() != ValueType::Int && Type() != ValueType::Float) {
    return false;
  }
  return true;
}
void SymbolValue::CheckOperatablity(const HaveFunCompiler::Parser::Parser::location_type &loc) {
  if (!HasOperatablity())
    throw TypeMismatchException(loc, TypeToString(), "int/float", "Underlying type should be int/float");
}

SymbolValue::ValueType SymbolValue::UnderlyingType() const {
  if (Type() == ValueType::Array) {
    auto ad = GetArrayDescriptor();
    if (!ad->dimensions.empty()) {
      throw std::runtime_error("Can't access array " + TypeToString());
    }
    return GetArrayDescriptor()->value_type;
  }
  return type;
}

std::string SymbolValue::TypeToTACString() const {
  std::string ret = std::string(magic_enum::enum_name<ValueType>(Type()));
  std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);

  switch (Type()) {
    case ValueType::Array: {
      ret = std::string(magic_enum::enum_name<ValueType>(GetArrayDescriptor()->value_type));
      std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
      size_t total_size = 1;
      for (auto d : GetArrayDescriptor()->dimensions) {
        total_size *= d;
      }
      ret += "[" + (total_size ? std::to_string(total_size) : "") + "]";
      break;
    }
    case ValueType::Parameters: {
      ret += ": ";
      auto param = GetParameters();
      ret += std::string(magic_enum::enum_name<ValueType>(param->get_return_type()));
      ret += "(";
      for (auto type : *param) {
        ret += type->value_.TypeToString() + ",";
      }
      if (param->empty()) {
        ret.push_back(')');
      } else {
        ret.back() = ')';
      }
    }
    default: {
      break;
    }
  }

  return ret;
}
SymbolValue::operator bool() const {
  switch (Type()) {
    case ValueType::Void:
      throw std::runtime_error("Void type can't be converted to bool!");
    case ValueType::Int:
      return GetInt() != 0;
    case ValueType::Float:
      return GetFloat() != 0;
    case ValueType::Parameters:
      return GetParameters() != nullptr;
    case ValueType::Str:
      return GetStr() != nullptr;
    case ValueType::Array: {
      auto arrayDescriptor = GetArrayDescriptor();
      if (!arrayDescriptor->dimensions.empty()) {
        throw std::runtime_error("Array can't be converted to bool!");
      }
      if (arrayDescriptor->subarray->empty()) {
        throw std::runtime_error("Null array");
      }
      return static_cast<bool>(arrayDescriptor->subarray->at(0)->ret->value_);
    }
    default:
      throw std::logic_error("Unknown ValueType " + std::string(magic_enum::enum_name<ValueType>(Type())));
  }
}

std::string Symbol::get_name() const {
  if (value_.Type() == SymbolValue::ValueType::Array) {
    auto arrayDescriptor = value_.GetArrayDescriptor();
    std::string name = arrayDescriptor->base_addr.lock()->get_tac_name(true);
    for (auto d : value_.GetArrayDescriptor()->dimensions) {
      name += "[" + std::to_string(d) + "]";
    }
    return name;
  }
  if (name_.has_value()) {
    std::string name = name_.value();
    switch (value_.Type()) {
      case SymbolValue::ValueType::Array: {
        for (auto d : value_.GetArrayDescriptor()->dimensions) {
          name += "[" + std::to_string(d) + "]";
        }
        break;
      }
      case SymbolValue::ValueType::Parameters: {
        auto param = value_.GetParameters();
        name += ": ";
        name += magic_enum::enum_name<SymbolValue::ValueType>(param->get_return_type());
        name += "(";
        for (auto sym : *param) {
          name += sym->value_.TypeToString() + ",";
        }
        if (param->empty()) {
          name.push_back(')');
        } else {
          name.back() = ')';
        }
        break;
      }

      default: {
        break;
      }
    }
    return name;
  }
  return value_.ToString();
}

std::string Symbol::get_tac_name(bool name_only) const {
  if (value_.Type() == SymbolValue::ValueType::Array && !name_only) {
    auto arrayDescriptor = value_.GetArrayDescriptor();
    std::string name = arrayDescriptor->base_addr.lock()->get_tac_name(true);
    return name + "[" + arrayDescriptor->base_offset->get_tac_name() + "]";
  }
  if (name_.has_value()) {
    std::string name = name_.value();
    return name;
  }
  return value_.ToString();
}

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler