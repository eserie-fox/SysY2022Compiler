#pragma once

#include <memory>

namespace HaveFunCompiler {
namespace ThreeAddressCode {
class ThreeAddressCodeList;
}
}  // namespace HaveFunCompiler

namespace HaveFunCompiler {
namespace AssemblyBuilder {
using TACList = HaveFunCompiler::ThreeAddressCode::ThreeAddressCodeList;
using TACListPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCodeList>;
}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler