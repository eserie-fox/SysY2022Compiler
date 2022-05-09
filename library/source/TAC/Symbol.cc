#include "TAC/Symbol.hh"
#include <cassert>
#include <cmath>
#include <exception>
#include <stdexcept>
#include <string>
#include "MagicEnum.hh"

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
    throw std::runtime_error("SymbolValue::GetFloat (Actually " + std::string(magic_enum::enum_name<ValueType>(type)) +
                             ")");
  }
  return std::get<float>(value);
}
int SymbolValue::GetInt() const {
  if (type != ValueType::Int) {
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
    if (type == ValueType::Float) {
      float OtherVal;
      OtherVal = other.GetFloat();
      return SymbolValue((int)(ThisVal < OtherVal));
    } else {
      int OtherVal = other.GetInt();
      return SymbolValue((int)(ThisVal < OtherVal));
    }
  } else {
    int ThisVal = GetInt();
    if (type == ValueType::Float) {
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
    if (type == ValueType::Float) {
      float OtherVal;
      OtherVal = other.GetFloat();
      return SymbolValue((int)(ThisVal == OtherVal));
    } else {
      int OtherVal = other.GetInt();
      return SymbolValue((int)(ThisVal == OtherVal));
    }
  } else {
    int ThisVal = GetInt();
    if (type == ValueType::Float) {
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
    if (type == ValueType::Float) {
      float OtherVal;
      OtherVal = other.GetFloat();
      return SymbolValue((int)(ThisVal && OtherVal));
    } else {
      int OtherVal = other.GetInt();
      return SymbolValue((int)(ThisVal && OtherVal));
    }
  } else {
    int ThisVal = GetInt();
    if (type == ValueType::Float) {
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
    if (type == ValueType::Float) {
      float OtherVal;
      OtherVal = other.GetFloat();
      return SymbolValue((int)(ThisVal || OtherVal));
    } else {
      int OtherVal = other.GetInt();
      return SymbolValue((int)(ThisVal || OtherVal));
    }
  } else {
    int ThisVal = GetInt();
    if (type == ValueType::Float) {
      float OtherVal;
      OtherVal = other.GetFloat();
      return SymbolValue((int)(ThisVal || OtherVal));
    } else {
      int OtherVal = other.GetInt();
      return SymbolValue((int)(ThisVal || OtherVal));
    }
  }
}

SymbolValue::operator bool() const {
  if (type == ValueType::Void) {
    throw std::runtime_error("Void type can't be converted to bool!");
  }
  if (type == ValueType::Int) {
    return GetInt() != 0;
  }
  if (type == ValueType::Float) {
    return GetFloat() != 0;
  }
  if (type == ValueType::Parameters) {
    return GetParameters() != nullptr;
  }
  if (type == ValueType::Str) {
    return GetStr() != nullptr;
  }
  return false;
}

std::string Symbol::get_name() const {
  if (name_.has_value()) {
    return name_.value();
  }
  return value_.ToString();
}

}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler