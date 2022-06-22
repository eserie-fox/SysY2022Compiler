#pragma once

#include <memory>

namespace HaveFunCompiler {
namespace ThreeAddressCode {
class ThreeAddressCodeList;
struct ThreeAddressCode;
struct Symbol;
}  // namespace ThreeAddressCode
}  // namespace HaveFunCompiler

namespace HaveFunCompiler {
namespace AssemblyBuilder {
using SymbolPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::Symbol>;
using TACPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>;
using TACList = HaveFunCompiler::ThreeAddressCode::ThreeAddressCodeList;
using TACListPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCodeList>;
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler