#include <stdexcept>
#include <string>

namespace HaveFunCompiler {

static std::string AppendPrefixIfNotEmpty(std::string prefix, std::string msg) {
  if (msg.empty()) return "";
  return prefix + msg;
}
using Exception = std::exception;
class LogicException : public std::logic_error {
 public:
  using std::logic_error::logic_error;
};
class RuntimeException : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class TypeMismatchException : public RuntimeException {
 public:
  TypeMismatchException(std::string actual_type, std::string expected_type, std::string msg = "")
      : RuntimeException("Type mismatch error: " + msg + "\nExpected type '" + expected_type + "' Actual type '" +
                         actual_type + "'") {}
};
class RedefinitionException : public RuntimeException {
 public:
  RedefinitionException(std::string redef_indent)
      : RuntimeException("Redefinition error: '" + redef_indent + "' has already been defined") {}
};
class UnknownException : public RuntimeException {
 public:
  UnknownException(std::string msg = "") : RuntimeException("Unkown error" + AppendPrefixIfNotEmpty(": ", msg)) {}
};

class NullReferenceException : public LogicException {
 public:
  NullReferenceException(const char *file, int line, std::string msg = "")
      : LogicException("NullReference error at [" + std::string(file) + ":" + std::to_string(line) + "]" +
                       AppendPrefixIfNotEmpty(": ", msg)) {}
};

class OutOfRangeException : public LogicException {
 public:
  OutOfRangeException(const char *file, int line, std::string msg = "")
      : LogicException("OutOfRange error at [" + std::string(file) + ":" + std::to_string(line) + "]" +
                       AppendPrefixIfNotEmpty(": ", msg)) {}
};

}  // namespace HaveFunCompiler
