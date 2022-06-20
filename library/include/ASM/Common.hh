#pragma once

#include <memory>

namespace HaveFunCompiler {
namespace ThreeAddressCode {
class ThreeAddressCodeList;
struct ThreeAddressCode;
struct Symbol;
}
}  // namespace HaveFunCompiler

namespace HaveFunCompiler {
namespace AssemblyBuilder {
using TACList = HaveFunCompiler::ThreeAddressCode::ThreeAddressCodeList;
using TACListPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCodeList>;
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler