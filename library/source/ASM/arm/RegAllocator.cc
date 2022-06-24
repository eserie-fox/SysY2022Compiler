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
            switch (tac->a_->value_.Type())
            {
            case SymbolValue::ValueType::Array:
            case SymbolValue::ValueType::Int:
                intParam.push_back(tac->a_);
                break;
            
            case SymbolValue::ValueType::Float:
                floatParam.push_back(tac->a_);
                break;

            default:
                throw std::runtime_error("RegAllocator: unrecognized parameter symbol!\n");
                break;
            }
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

void RegAllocator::LinearScan(std::vector<LiveinfoWithSym> &liveSymLs)
{
    // 活跃区间列表，按照起点升序排序
    std::sort(liveSymLs.begin(), liveSymLs.end(), [](const LiveinfoWithSym &x,const LiveinfoWithSym &y){
        return x.liveInfo->endPoints < y.liveInfo->endPoints;
    });
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