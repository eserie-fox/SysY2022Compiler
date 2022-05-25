#include <stdexcept>
#include <string>
#include "HFParser.hh"

namespace HaveFunCompiler {

static std::string AppendPrefixIfNotEmpty(std::string prefix, std::string msg) {
  if (msg.empty()) return "";
  return prefix + msg;
}
using Exception = HaveFunCompiler::Parser::Parser::syntax_error;
class LogicException : public Exception {
 public:
  using Exception::Exception;
};
class RuntimeException : public Exception {
 public:
  using Exception::Exception;
};

class TypeMismatchException : public RuntimeException {
 public:
  // using RuntimeException::RuntimeException;
  TypeMismatchException(const HaveFunCompiler::Parser::Parser::location_type &loc, std::string actual_type,
                        std::string expected_type, std::string msg = "")
      : RuntimeException(loc, "Type mismatch error: " + msg + "\nExpected type '" + expected_type + "' Actual type '" +
                                  actual_type + "'") {}
};
class RedefinitionException : public RuntimeException {
 public:
  // using RuntimeException::RuntimeException;
  RedefinitionException(const HaveFunCompiler::Parser::Parser::location_type &loc, std::string redef_indent)
      : RuntimeException(loc, "Redefinition error: '" + redef_indent + "' has already been defined") {}
};
class UnknownException : public RuntimeException {
 public:
  // using RuntimeException::RuntimeException;
  UnknownException(const HaveFunCompiler::Parser::Parser::location_type &loc, std::string msg = "")
      : RuntimeException(loc, "Unkown error" + AppendPrefixIfNotEmpty(": ", msg)) {}
};

class NullReferenceException : public LogicException {
 public:
  // using LogicException::LogicException;
  NullReferenceException(const HaveFunCompiler::Parser::Parser::location_type &loc, const char *file, int line,
                         std::string msg = "")
      : LogicException(loc, "NullReference error at [" + std::string(file) + ":" + std::to_string(line) + "]" +
                                AppendPrefixIfNotEmpty(": ", msg)) {}
};

class OutOfRangeException : public LogicException {
 public:
  // using LogicException::LogicException;
  OutOfRangeException(const HaveFunCompiler::Parser::Parser::location_type &loc, const char *file, int line,
                      std::string msg = "")
      : LogicException(loc, "OutOfRange error at [" + std::string(file) + ":" + std::to_string(line) + "]" +
                                AppendPrefixIfNotEmpty(": ", msg)) {}
};

}  // namespace HaveFunCompiler
