#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include "ASM/Common.hh"
#include "MacroUtil.hh"

namespace HaveFunCompiler{
namespace AssemblyBuilder{

class LiveAnalyzer;
struct SymLiveInfo;

/*
 * 函数参数通过r0-r3, s0-s15传递
 * 无法用寄存器传递的参数，从右向左压栈
 * 
 * 通用寄存器可用的:r0-r12
 * 浮点寄存器可用的:s0-s31
 * 
 * 通用和浮点寄存器各保留2个不分配，用作计算的临时存储
 * 规则：参数传递所用的最大寄存器的后2个保留。如参数使用r0, r1传递，则保留r2, r3不分配
*/

class RegAllocator
{
public:

    using SymPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::Symbol>;
    using iterator = std::list<std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>>::iterator;
    using const_iterator = std::list<std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>>::const_iterator;
    using TACOperationType = HaveFunCompiler::ThreeAddressCode::TACOperationType;

    // stack_param的偏移(value)，正数，第一个为0
    // stack_var的偏移，负数，基于栈基址
    struct SymAttribute
    {
        // 对于函数，代表栈帧大小
        // 对于变量，代表寄存器编号或栈基址偏移
        int value;
        union
        {
            enum {INT_REG, FLOAT_REG, STACK_VAR, STACK_PARAM, NOT_USED} store_type;  // 变量的存储类型
            struct {
                uint32_t intRegs, floatRegs;
            } used_regs;  // 函数中已分配的寄存器集
        } attr;
    };

    struct LiveinfoWithSym
    {
        const SymLiveInfo *liveInfo;
        SymPtr sym;

        LiveinfoWithSym(const SymLiveInfo *liveInfoPtr, SymPtr symPtr) : liveInfo(liveInfoPtr), sym(symPtr) {};
    };

    NONCOPYABLE(RegAllocator)
    RegAllocator(const LiveAnalyzer&);

    SymAttribute get_SymAttribute(SymPtr sym);

private:

    // 函数 对应栈和寄存器使用属性
    // 变量 对应地址属性
    // 根据函数或变量Sym，取得它们的属性
    std::unordered_map<SymPtr, SymAttribute> symAttrMap;

    // 函数内的局部变量，整型参数，浮点型参数的列表
    std::vector<SymPtr> localSym, intParam, floatParam;

    // 栈上数组(栈上数组由数组指针Sym指向)
    // 根据指针Sym，取得数组地址属性
    std::unordered_map<SymPtr, SymAttribute> ptrToArrayOnStack;

    static const int intRegPoolSize = 13, floatRegPoolSize = 32;
    static const int intRegParamUseNumber = 4, floatRegParamUseNumber = 16;
    // static const int intRegPool[intRegPoolSize], floatRegPoolSize[floatRegPoolSize];

private:
    // 得到局部变量、整型参数和浮点型参数列表，建立指针到栈上数组的映射
    void ContextInit(const LiveAnalyzer&);

    // 线性扫描，分配寄存器与栈空间
    void LinearScan(std::vector<LiveinfoWithSym>& liveSymLs);
};


}
}