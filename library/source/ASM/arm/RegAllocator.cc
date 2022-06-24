#include "ASM/arm/RegAllocator.hh"
#include "ASM/LiveAnalyzer.hh"
#include "TAC/ThreeAddressCode.hh"
#include "TAC/Symbol.hh"
#include <vector>
#include <queue>
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

RegAllocator::SymAttribute RegAllocator::LinearScan(std::vector<LiveinfoWithSym> &liveSymLs)
{
    SymAttribute funcAttr;
    int intRegUsedNumber = 0, floatRegUsedNumber = 0;  // 值为已使用的寄存器个数，也是下一个要使用的寄存器编号

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

    /* 为函数参数分配寄存器和栈空间  */
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
        }
    }
    /*  函数参数分配寄存器和栈空间 完成  */

    // 此时intRegUsedNumber, floatRegUsedNumber中的值为参数使用的寄存器个数
    // 为函数保留2个通用和浮点寄存器做临时运算用
    

    // 局部变量活跃区间列表，按照起点升序排序
    std::sort(liveSymLs.begin(), liveSymLs.end(), [](const LiveinfoWithSym &x,const LiveinfoWithSym &y){
        return x.liveInfo->endPoints < y.liveInfo->endPoints;
    });

    return funcAttr;
}

RegAllocator::RegAllocator(const LiveAnalyzer& liveAnalyzer)
{
    // 得到函数中的局部变量、整型参数和浮点型参数
    ContextInit(liveAnalyzer);

    // 将局部变量与其活跃区间绑定
    std::vector<LiveinfoWithSym> liveSymLs;
    for (auto sym : localSym)
    {
        auto liveInfoPtr = liveAnalyzer.get_symLiveInfo(sym);
        if (!liveInfoPtr)
            throw std::runtime_error("LiveAnalyzer fault: there are local variables that are not analyzed.");
        liveSymLs.emplace_back(liveInfoPtr, sym);
    }

    LinearScan(liveSymLs);
}

RegAllocator::SymAttribute RegAllocator::get_SymAttribute([[maybe_unused]] SymPtr sym) 
{ 
    return {}; 
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler