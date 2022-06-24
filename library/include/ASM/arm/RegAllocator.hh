#pragma once

#include <cstdint>
#include <vector>
#include <list>
#include <set>
#include <unordered_map>
#include "ASM/Common.hh"
#include "MacroUtil.hh"

namespace HaveFunCompiler {
namespace ThreeAddressCode {
enum class TACOperationType;
}
}  // namespace HaveFunCompiler

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
 * 规则：保留r0，以及参数传递所用的最大寄存器的后1个寄存器。如参数使用r0, r1传递，则保留r0, r2不分配
 * 浮点寄存器保留规则同理
*/

/* 
 * 一个活跃区间由一个或多个dfn连续的range组成 
 * 不同的range，虽然dfn不连续，但控制流可能是连续的
 * 例如：分支出两条路径，其中一条路径p的dfn肯定不与父路径连续，但控制流可能直接跳转到p
 * 
 * 也就是说，range的起点不一定是def，终点不一定是use。
 * 如果不同的range分配不同的地址/寄存器，控制流在range之间直接跳转时就发生了地址冲突。
 * 即使是不同的range分配相同的地址，也会发生冲突。
 * 
 * 这里 由LiveAnalyzer保证，同一变量的活跃区间内的每一个range，起点一定是def，终点一定是use。
*/
class RegAllocator
{
public:

    using SymPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::Symbol>;
    using iterator = std::list<std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>>::iterator;
    using const_iterator = std::list<std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>>::const_iterator;
    using TACOperationType = HaveFunCompiler::ThreeAddressCode::TACOperationType;


    struct SymAttribute
    {
        // 更改：移除NOT_USED类型
        enum StoreType {INT_REG, FLOAT_REG, STACK};

        // 对于函数，代表局部变量消耗的栈帧大小(保证8字节对齐)
        // 对于变量，代表寄存器编号或栈基址偏移
        // 分配在栈上的函数参数的偏移，正数，按地址增长方向的偏移，从右往左压栈，即第一个为0，第二个为4
        // stack_var的偏移，负数，基于栈基址
        int value;

        union
        {
            StoreType store_type;  // 变量的存储类型
            struct {
                uint32_t intRegs, floatRegs;  // 函数中已分配的寄存器集
                int intReservedReg, floatReservedReg;  // 额外保留的一个寄存器号(即除了r0/s0的另一个)
            } used_regs; 
        } attr;

        SymAttribute()
        {
            value = 0;
            attr.used_regs = {0, 0, 0, 0};
        }
    };

    struct LiveinfoWithSym
    {
        const SymLiveInfo *liveInfo;
        SymPtr sym;
        bool canSpill;  // 是否能够进行溢出

        LiveinfoWithSym(const SymLiveInfo *liveInfoPtr, SymPtr symPtr, bool can_spill = true) : liveInfo(liveInfoPtr), sym(symPtr), canSpill(can_spill) {};
    };

    NONCOPYABLE(RegAllocator)
    RegAllocator(const LiveAnalyzer&);

    SymAttribute get_SymAttribute(SymPtr sym);

private:

    // 函数 对应栈和寄存器使用属性
    // 变量 对应地址属性
    // 根据函数或变量Sym，取得它们的属性
    std::unordered_map<SymPtr, SymAttribute> symAttrMap;

    // 函数内的局部变量，参数的列表
    std::vector<SymPtr> localSym, paramLs;

    // 栈上数组(栈上数组由数组指针Sym指向)
    // 根据指针Sym，取得数组地址属性
    std::unordered_map<SymPtr, SymAttribute> ptrToArrayOnStack;


    enum ParamType {INT_PARAM, FLOAT_PARAM};

    static const int intRegPoolSize = 13, floatRegPoolSize = 32;
    static const int intRegParamUsableNumber = 4, floatRegParamUsableNumber = 16;
    // static const int intRegPool[intRegPoolSize], floatRegPoolSize[floatRegPoolSize];

private:
    // 得到局部变量、整型参数和浮点型参数列表，建立指针到栈上数组的映射
    void ContextInit(const LiveAnalyzer&);

    // 线性扫描，分配寄存器与栈空间
    // 返回函数属性
    SymAttribute LinearScan(std::vector<SymPtr>& localSym, const LiveAnalyzer& liveAnalyzer);


};


}
}