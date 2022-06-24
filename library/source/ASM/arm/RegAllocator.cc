#include "ASM/arm/RegAllocator.hh"
#include "ASM/LiveAnalyzer.hh"
#include "TAC/ThreeAddressCode.hh"
#include "TAC/Symbol.hh"
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace HaveFunCompiler {
namespace AssemblyBuilder {

using namespace HaveFunCompiler::ThreeAddressCode;

// const int RegAllocator::intRegPool[RegAllocator::intRegPoolSize] = {
    
// };

// const int RegAllocator::floatRegPoolSize[RegAllocator::floatRegPoolSize] = {

// };

void RegAllocator::ContextInit(const LiveAnalyzer& liveAnalyzer)
{
    if (intRegParamUsableNumber + 2 >= intRegPoolSize || floatRegParamUsableNumber + 2 >= floatRegPoolSize) 
        throw std::runtime_error("RegAllocator: Register config error");

    for (auto it = liveAnalyzer.get_fbegin(); it != liveAnalyzer.get_fend(); ++it)
    {
        auto &tac = *it;
        if (tac->operation_ == TACOperationType::Parameter)  // tac是形参声明
        {
            if (!(tac->a_))
                throw std::runtime_error("RegAllocator: parameter TAC get symbol fault!");
            paramLs.push_back(tac->a_);
        }

        else if (tac->operation_ == TACOperationType::Constant || tac->operation_ == TACOperationType::Variable)  // tac是局部变量声明
        {
            if (!(tac->a_))
                throw std::runtime_error("RegAllocator: Declare TAC get symbol fault!");
            localSym.push_back(tac->a_);
            if (tac->a_->value_.Type() == SymbolValue::ValueType::Array)
                ptrToArrayOnStack.emplace(tac->a_, SymAttribute());
        }
    }
}

RegAllocator::SymAttribute RegAllocator::LinearScan(std::vector<SymPtr>& localSym, const LiveAnalyzer& liveAnalyzer)
{
    SymAttribute funcAttr;
    
    // 活跃区间端点[begin, end]，按end排序，end相同时begin小的对象偏序更大
    auto _cmp = [](const LiveinfoWithSym &x, const LiveinfoWithSym &y)
    {
        if (x.liveInfo->endPoints.second < y.liveInfo->endPoints.second)
            return true;
        else if (x.liveInfo->endPoints.second == y.liveInfo->endPoints.second)
            return x.liveInfo->endPoints.first > y.liveInfo->endPoints.first;
        else
            return false;
    };

    auto getParamType = [](SymPtr param)
    {
        if (param->value_.Type() == SymbolValue::ValueType::Int || param->value_.Type() == SymbolValue::ValueType::Array)
            return INT_PARAM;
        else if (param->value_.Type() == SymbolValue::ValueType::Float)
            return FLOAT_PARAM;
        else
        {
            throw std::runtime_error("RegAllocator: Unrecognized parameter type!");
            return INT_PARAM;
        }
    };

    // 当前已分配在寄存器中的变量(包括参数和局部变量)
    std::set<LiveinfoWithSym, decltype(_cmp)> symInReg(_cmp);

    /* 为函数参数分配寄存器和栈空间  */
    int intRegUsedNumber = 0, floatRegUsedNumber = 0;  // 值为已使用的寄存器个数，也是下一个要使用的寄存器编号
    int paramStackOffset = 0;
    for (auto param : paramLs)
    {
        int regUsedNumber, regParamUsableNumber;  // 使用这两个变量，对int和float类型参数统一处理
        ParamType paramType = getParamType(param);
        
        if (paramType == INT_PARAM)
        {
            regUsedNumber = intRegUsedNumber;
            regParamUsableNumber = intRegParamUsableNumber;
        }
        else
        {
            regUsedNumber = floatRegUsedNumber;
            regParamUsableNumber = floatRegParamUsableNumber;
        }

        if (symAttrMap.emplace(param, SymAttribute()).second == false)
            throw std::runtime_error("RegAllocator: same parameter appears multiple times!");
        auto &paramAttribute = symAttrMap[param];

        // 能够被用于传递参数的寄存器已经用完了，参数应使用栈传递
        if (regUsedNumber >= regParamUsableNumber)
        {
            paramAttribute.attr.store_type = SymAttribute::StoreType::STACK;
            paramAttribute.value = paramStackOffset;
            paramStackOffset += 4;   // 一个参数，占用4字节
        }

        // 否则，分配在寄存器中
        else
        {
            paramAttribute.attr.store_type = paramType == INT_PARAM ? SymAttribute::StoreType::INT_REG : SymAttribute::StoreType::FLOAT_REG;
            paramAttribute.value = regUsedNumber;
            if (paramType == INT_PARAM)
            {
                SET_UINT(funcAttr.attr.used_regs.intRegs, intRegUsedNumber);
                ++intRegUsedNumber;
            }
            else
            {
                SET_UINT(funcAttr.attr.used_regs.floatRegs, floatRegUsedNumber);
                ++floatRegUsedNumber;
            }

            auto liveInfo = liveAnalyzer.get_symLiveInfo(param);
            if (!liveInfo)
                throw std::runtime_error("LiveAnalyzer fault: there are parameters that are not analyzed.");
            // 对于分配在寄存器中的函数参数，在线性扫描时不允许溢出操作
            symInReg.emplace(liveInfo, param, false);
        }
    }
    /*  函数参数分配寄存器和栈空间 完成  */

    // 此时intRegUsedNumber, floatRegUsedNumber中的值为参数使用的寄存器个数
    // 为函数再保留1个通用和浮点寄存器做临时运算用
    SET_UINT(funcAttr.attr.used_regs.intRegs, intRegUsedNumber);
    funcAttr.attr.used_regs.intReservedReg = intRegUsedNumber;
    SET_UINT(funcAttr.attr.used_regs.floatRegs, floatRegUsedNumber);
    funcAttr.attr.used_regs.floatReservedReg = floatRegUsedNumber;
    ++intRegUsedNumber, ++floatRegUsedNumber;
    

    /* 开始为局部变量(虚拟寄存器)分配寄存器和栈空间 */

    // 初始化可分配的寄存器
    std::vector<int> intRegPool, floatRegPool;
    for (int i = intRegPoolSize - 1; i >= intRegUsedNumber; --i)
        intRegPool.push_back(i);
    for (int i = floatRegPoolSize - 1; i >= floatRegUsedNumber; --i)
        floatRegPool.push_back(i);

    // 构建局部变量活跃区间的列表
    std::vector<LiveinfoWithSym> localSymLiveInfo;
    for (auto sym : localSym)
    {
        auto liveInfo = liveAnalyzer.get_symLiveInfo(sym);
        if (!liveInfo)
            throw std::runtime_error("LiveAnalyzer fault: there are local variables that are not analyzed.");
        localSymLiveInfo.emplace_back(liveInfo, sym);
    }

    // 局部变量的活跃区间列表按照起点进行升序排序
    std::sort(localSymLiveInfo.begin(), localSymLiveInfo.end(), [](const LiveinfoWithSym &x,const LiveinfoWithSym &y){
        return x.liveInfo->endPoints < y.liveInfo->endPoints;
    });

    

    return funcAttr;
}

RegAllocator::RegAllocator(const LiveAnalyzer& liveAnalyzer)
{
    // 得到函数中的局部变量、参数列表
    ContextInit(liveAnalyzer);

    // // 将局部变量与其活跃区间绑定
    // std::vector<LiveinfoWithSym> liveSymLs;
    // for (auto sym : localSym)
    // {
    //     auto liveInfoPtr = liveAnalyzer.get_symLiveInfo(sym);
    //     if (!liveInfoPtr)
    //         throw std::runtime_error("LiveAnalyzer fault: there are local variables that are not analyzed.");
    //     liveSymLs.emplace_back(liveInfoPtr, sym);
    // }

    LinearScan(localSym, liveAnalyzer);
}

RegAllocator::SymAttribute RegAllocator::get_SymAttribute([[maybe_unused]] SymPtr sym) 
{ 
    return {}; 
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler