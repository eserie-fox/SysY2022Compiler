#pragma once

#include <cstdint>
#include "ASM/Common.hh"
/*
 * 需要信息：
 * 活跃区间到变量的映射
 * 活跃区间列表（按start从小到大排）
 * 已经进行分配的活跃区间（按end从大到小排）
 * 
 * 输出信息：
 * 给定一个变量，返回该变量的地址描述符
 * 返回这个函数的属性描述符
*/

namespace HaveFunCompiler{
namespace AssemblyBuilder{

class LiveAnalyzer;

class RegAllocator
{
public:

    using SymPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::Symbol>;

    struct SymAttribute
    {
        // 对于函数，代表栈帧大小
        // 对于变量，代表寄存器编号或栈基址偏移
        int value;
        union
        {
            enum {INT_REG, FLOAT_REG, STACK} store_type;  // 变量的存储类型
            struct {
                uint32_t intRegs, floatRegs;
            } used_regs;  // 函数中已分配的寄存器集
        } attr;
    };

    RegAllocator(const LiveAnalyzer&);

    SymAttribute get_SymAttribute(SymPtr sym);
};


}
}