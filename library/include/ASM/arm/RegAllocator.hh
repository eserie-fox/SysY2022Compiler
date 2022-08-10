#pragma once

#include <cstdint>
#include <climits>
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

class LiveIntervalAnalyzer;
struct SymLiveInfo;

using LiveInterval = std::pair<size_t, size_t>;

struct SymAttribute
{
    // 更改：移除NOT_USED类型
    enum StoreType {INT_REG, FLOAT_REG, STACK_VAR, STACK_PARAM};

    // 对于函数，代表局部变量消耗的栈帧大小(保证8字节对齐)
    // 对于变量，代表寄存器编号或栈上偏移
    // 分配在栈上的函数参数的偏移，正数，按地址增长方向的偏移，从右往左压栈，即第一个为0，第二个为4
    // 栈上局部变量的偏移，正数，基于栈顶地址
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
 * 或者，固定保留r0, r4, s0, s16？
*/


/* 
 * 使用线性扫描算法分配寄存器，分配器依赖于活跃分析的结果，原理：
 * (下述地址是指寄存器和栈空间的统称)
 * 
 * 一个活跃区间由一个或多个dfn连续的活跃range组成
 * 不同的range，dfn不连续，但对于同一变量，PC可能从一个range内直接转移到另一个range内，
 * 例如：分支出两条路径，其中一条路径p的dfn肯定不与父路径连续，但控制流可能从父路径转移到p。
 * 
 * 同一变量的不同range如果分配不同地址，控制直接转移时会发生冲突，处理冲突需要增加指令，暂不考虑这种方案。
 * (冲突举例：range1变量分配在地址A，range2变量分配在地址B，PC从range1转移到range2时缺少把数据从A拷贝到B的过程)
 * 
 * 那么，同一变量的所有range分配相同地址。考虑这种情况下，在range之间的非活跃点该地址是否可以分配给其他变量：
 * 站在活跃性的角度可以得到控制流的属性，总结：
 * 1. PC可以在单个变量的不同range之间转移，也可以从range内转移到非活跃点
 * 2. PC可以从相对于某个变量的非活跃点跳转到range内的def点
 *    (如果存在从非活跃点转移到range内部的控制流，根据活跃分析计算过程，range内部该点一定是个def，
 *    否则变量也在该非活跃点活跃，矛盾。)
 * 
 * 根据上述属性能够证明：不同的变量，只要它们活跃区间的range没有交集，就可以分配相同的地址。
 * 因为，range没有交集的变量集合中，
 * 任意变量的任意一个活跃点，一定是其他所有变量的非活跃点(range无交集)，同一程序点只可能有一个变量活跃。
 * 所以地址资源能够被不同变量共享，不存在同一程序点有多个变量活跃且都占有同一地址的情况。
 * 
 * 然后考虑该地址内容是否会产生冲突，即既然共享了地址，那在程序运行过程中，
 * 使用这个变量的值时(此时该变量是活跃的)，能否保证该地址内的值是这个变量自己的，而不是其他变量的。
 * 
 * 根据上述控制流的属性，对于range不相交的变量集合，要使一个变量活跃，程序一定最先转移到该变量的定值(第2个属性)。
 * 这个定值会把该变量的值写入它的地址，在某个变量活跃时，
 * 定值保证了地址内是该变量的正确值，而不是其它变量的值，即使这些变量共用这个地址，不会产生冲突。
*/
class RegAllocator
{
public:

    using SymPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::Symbol>;
    using iterator = std::list<std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>>::iterator;
    using const_iterator = std::list<std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>>::const_iterator;
    using TACOperationType = HaveFunCompiler::ThreeAddressCode::TACOperationType;

    // struct LiveinfoWithSym
    // {
    //     const SymLiveInfo *liveInfo;
    //     SymPtr sym;
    //     bool canSpill;  // 是否能够进行溢出

    //     LiveinfoWithSym(const SymLiveInfo *liveInfoPtr, SymPtr symPtr, bool can_spill = true) : liveInfo(liveInfoPtr), sym(symPtr), canSpill(can_spill) {};
    // };

    NONCOPYABLE(RegAllocator)

    RegAllocator(const LiveIntervalAnalyzer&);

    SymAttribute get_SymAttribute(SymPtr sym);
    SymAttribute get_ArrayAttribute(SymPtr arrPtr);

private:

    // 函数 对应栈和寄存器使用属性
    // 变量 对应地址属性
    // 根据函数或变量Sym，取得它们的属性
    std::unordered_map<SymPtr, SymAttribute> symAttrMap;

    // 函数属性
    SymAttribute funcAttr;

    // 函数内的局部变量，参数的列表
    std::vector<SymPtr> localSym, paramLs;

    // 栈上数组(栈上数组由数组指针Sym指向)
    // 根据指针Sym，取得数组地址属性
    std::unordered_map<SymPtr, SymAttribute> ptrToArrayOnStack;

private:
    enum SymType {PARAM, LOCAL_VAR};
    enum SymValueType {INT, FLOAT};
//    enum ParamType {INT_PARAM, FLOAT_PARAM};

    // 寄存器分配器维护的每个变量的信息
    struct SymInfo
    {
        SymPtr symPtr;  
        SymType symType;  // 变量的类型：函数参数或局部变量
        SymValueType symValueType;  // 变量的值类型，int或float
        const std::set<LiveInterval> *liveRanges;  // 变量的活跃范围列表
        size_t spillCost;  // 变量溢出到栈的代价

        SymInfo(SymPtr symbol, SymType type, SymValueType valueType, const std::set<LiveInterval> *liveRangesPtr, size_t spill_cost) :
            symPtr(symbol), symType(type), symValueType(valueType), liveRanges(liveRangesPtr), spillCost(spill_cost) {}

        bool operator<(const SymInfo& o) const
        {
            return spillCost < o.spillCost;
        }

        // 由定值计数和使用计数计算溢出代价
        // def：从栈中读取，再写回栈中，代价权重为2
        // use：从栈中读取，代价权重为1
        static size_t calculateSpillCost(size_t defCnt, size_t useCnt)
        {
            if ((SIZE_MAX - useCnt) / 2 < defCnt)
                return SIZE_MAX;
            return 2 * defCnt + useCnt;
        }
    };

    // 寄存器分配过程维护的每个寄存器的信息
    struct RegInfo
    {
        std::set<LiveInterval> occupyRanges;  // 寄存器被占用的区间集合
        int id;  // 寄存器号

        RegInfo(int _id) : id(_id) {}

        // 将寄存器分配给变量
        // 参数：该变量的活跃ranges
        // 成功分配返回true，冲突返回false
        bool AllocToSym(const std::set<LiveInterval> &symRanges);

    private:
        // 判断变量的活跃范围是否与物理寄存器当前的活跃范围冲突
        bool IsConflict(const std::set<LiveInterval> &symRanges);

        // 将src添加到occupyRanges中
        // 调用方需要确保不冲突
        void AddRanges(const std::set<LiveInterval>& src);
    };

public:
    static const int intRegPoolSize = 13, floatRegPoolSize = 32;
    static const int intRegParamUsableNumber = 4, floatRegParamUsableNumber = 16;
    // static const int intRegPool[intRegPoolSize], floatRegPoolSize[floatRegPoolSize];

private:
    // 得到局部变量、参数列表，建立指针到栈上数组的映射
    void ContextInit(const LiveIntervalAnalyzer&);

    // 得到每个参数传入时占用的寄存器或栈空间
    // 保存在该参数SymPtr对应的SymAttribute中
    void getParamAddr();

    /* 
     * 线性扫描，为函数参数和局部变量分配寄存器与栈空间
     * 可能会把函数参数重定位，
     * 即通过寄存器传入的参数，可能分配到局部变量栈上，通过栈传递的参数，可能分配到寄存器中。
     * 保证参数地址的重定位不会产生循环引用。
     * 
    */
    void LinearScan(const LiveIntervalAnalyzer& LiveIntervalAnalyzer);

    // 封装线性扫描时获取SymAttribute的过程
    // sym为参数，直接获取
    // sym为局部变量，在SymAttrMap中创建后返回
    SymAttribute& fetchSymAttr(const SymInfo &symInfo);

    static SymValueType fetchSymValueType(SymPtr sym);

    SymAttribute fetchAttr(SymPtr, std::unordered_map<SymPtr, SymAttribute>&);
};


}
}