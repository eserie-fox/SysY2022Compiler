#include "ASM/arm/RegAllocator.hh"
#include "ASM/LiveAnalyzer.hh"
#include "TAC/ThreeAddressCode.hh"
#include "TAC/Symbol.hh"
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
    if (intRegParamUsableNumber + 2 >= intRegPoolSize || floatRegParamUsableNumber + 2 >= floatRegPoolSize) 
        throw std::runtime_error("RegAllocator: Register config error");

    auto it = liveAnalyzer.get_fbegin();
    auto fend = liveAnalyzer.get_fend();
    std::unordered_set<SymPtr> tmpParams;

    // 提取参数列表
    for (int i = 0; i < 2 && it != fend; ++i, ++it);
    for (;it != fend && (*it)->operation_ == TACOperationType::Parameter; ++it)
    {
        auto &tac = *it;
        if (!(tac->a_))
            throw std::runtime_error("RegAllocator: parameter TAC get symbol fault!");
        paramLs.push_back(tac->a_);
        tmpParams.insert(tac->a_);  
    }

    // 提取局部变量列表
    auto& allSymSet = liveAnalyzer.get_allSymSet();
    for (auto sym : allSymSet)
    {
        if (tmpParams.find(sym) == tmpParams.end() && !sym->IsGlobal())
        {
            localSym.push_back(sym);
            if (sym->value_.Type() == SymbolValue::ValueType::Array)
                ptrToArrayOnStack.emplace(sym, SymAttribute());
        }
    }

    // for (auto it = liveAnalyzer.get_fbegin(); it != liveAnalyzer.get_fend(); ++it)
    // {
    //     auto &tac = *it;
    //     if (tac->operation_ == TACOperationType::Parameter)  // tac是形参声明
    //     {
    //         if (!(tac->a_))
    //             throw std::runtime_error("RegAllocator: parameter TAC get symbol fault!");
    //         paramLs.push_back(tac->a_);
    //     }

    //     else if (tac->operation_ == TACOperationType::Constant || tac->operation_ == TACOperationType::Variable)  // tac是局部变量声明
    //     {
    //         if (!(tac->a_))
    //             throw std::runtime_error("RegAllocator: Declare TAC get symbol fault!");
    //         localSym.push_back(tac->a_);
    //         if (tac->a_->value_.Type() == SymbolValue::ValueType::Array)
    //             ptrToArrayOnStack.emplace(tac->a_, SymAttribute());
    //     }
    // }
}

RegAllocator::SymValueType RegAllocator::fetchSymValueType(SymPtr sym)
{
    if (sym->value_.Type() == SymbolValue::ValueType::Int || sym->value_.Type() == SymbolValue::ValueType::Array)
        return INT;
    else if (sym->value_.Type() == SymbolValue::ValueType::Float)
        return FLOAT;
    else
    {
        throw std::runtime_error("RegAllocator: Unrecognized sym value type!");
        return INT;
    }
}

void RegAllocator::getParamAddr()
{
    /* 计算函数参数传入时使用的寄存器和栈空间，记录在对应的SymAttribute中  */
    int intRegUsedNumber = 0, floatRegUsedNumber = 0;  // 值为已使用的寄存器个数，也是下一个要使用的寄存器编号
    int paramStackOffset = 0;
    for (auto param : paramLs)
    {
        int regUsedNumber, regParamUsableNumber;  // 使用这两个变量，对int和float类型参数统一处理
        SymValueType paramType = fetchSymValueType(param);
        
        if (paramType == INT)
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
            paramAttribute.attr.store_type = SymAttribute::StoreType::STACK_PARAM;
            paramAttribute.value = paramStackOffset;
            paramStackOffset += 4;   // 一个参数，占用4字节
        }

        // 否则，参数应在寄存器中
        // 由于第一个参数通过r0，s0传递，而r0, r4, s0, s16不能分配给变量
        // 所以对第一个参数特殊处理，记录它在PoolSize - 1中
        else
        {
            paramAttribute.attr.store_type = paramType == INT ? SymAttribute::StoreType::INT_REG : SymAttribute::StoreType::FLOAT_REG;
            if (paramType == INT)
            {
                if (regUsedNumber == 0)
                    regUsedNumber = intRegPoolSize - 1;
                ++intRegUsedNumber;
            }
            else
            {
                if (regUsedNumber == 0)
                    regUsedNumber = floatRegPoolSize - 1;
                ++floatRegUsedNumber;
            }
            paramAttribute.value = regUsedNumber;
        }
    }
}

SymAttribute& RegAllocator::fetchSymAttr(const SymInfo &symInfo)
{
    // 程序debug完成后，可以去掉这些异常处理(运行逻辑正确，则无论输入如何都不会进入异常分支)
    if (symInfo.symType == PARAM)
    {
        if (symAttrMap.find(symInfo.symPtr) == symAttrMap.end())
            throw std::runtime_error("getParamAttr error: param didn't be inserted into symAttrMap");
        else
            return symAttrMap[symInfo.symPtr];
    }
    else
    {
        if (symAttrMap.emplace(symInfo.symPtr, SymAttribute()).second == false)
            throw std::runtime_error("getLocalVarAttr error: local var has been inserted into symAttrMap which shouldn't happen");
        return symAttrMap[symInfo.symPtr];
    }
}

void RegAllocator::LinearScan(const LiveAnalyzer& liveAnalyzer)
{
    // 得到参数传入时占用的地址，同时为每个参数创建了Attribute对象，保存在symAttrMap中
    getParamAddr();

    // 构建待分配的变量表
    // 使用优先队列，溢出权重大的优先分配
    std::priority_queue<SymInfo> syms;
    for (auto var : localSym)
    {
        auto liveInfo = liveAnalyzer.get_symLiveInfo(var);
        if (!liveInfo)
            throw std::runtime_error("LiveAnalyzer fault: there are local variables that are not analyzed.");
        syms.emplace(var, LOCAL_VAR, fetchSymValueType(var), &liveInfo->liveIntervalSet, SymInfo::calculateSpillCost(liveInfo->defCnt, liveInfo->useCnt));
    }
    
    for (auto param : paramLs)
    {
        auto liveInfo = liveAnalyzer.get_symLiveInfo(param);
        if (!liveInfo)
            throw std::runtime_error("LiveAnalyzer fault: there are parameters that are not analyzed.");
        syms.emplace(param, PARAM, fetchSymValueType(param), &liveInfo->liveIntervalSet, SymInfo::calculateSpillCost(liveInfo->defCnt, liveInfo->useCnt));
    }


    // 构造可分配物理寄存器表
    // 保留r0, r4, s0, s16
    // 分配时从下标0开始向后检查reg是否可用
    // 保存函数参数的reg分配优先级低，为了尽量保证分配在寄存器内的参数不需移动到栈中
    // 对参数的特殊处理：通过寄存器传递的参数，要么在原来的寄存器中，要么在变量的栈上分配一块空间
    // 通过栈传递的参数，要么保持在原来的栈位置，要么被分配在一个寄存器中
    std::vector<RegInfo> intRegs, floatRegs;

    // Index: 快速由寄存器号索引到保存该寄存器信息的下标
    int intRegsIndex[intRegPoolSize], floatRegsIndex[floatRegPoolSize];

    // 便于int类型和float类型统一处理
    std::unordered_map<SymValueType, std::vector<RegInfo>*> typeRegMap = {
        {INT, &intRegs}, {FLOAT, &floatRegs}
    };  // 不能map引用(引用不可变)，指针代替
    std::unordered_map<SymValueType, int*> typeRegIndexMap = {
        {INT, intRegsIndex}, {FLOAT, floatRegsIndex}
    };

    for (int i = 5, j = 0; i < intRegPoolSize; ++i, ++j)
    {
        intRegs.emplace_back(i);
        intRegsIndex[i] = j;
    }
    for (int i = 3, j = intRegs.size(); i >= 1; --i, ++j)
    {
        intRegs.emplace_back(i);
        intRegsIndex[i] = j;
    }
    for (int i = 17, j = 0; i < floatRegPoolSize; ++i, ++j)
    {
        floatRegs.emplace_back(i);
        floatRegsIndex[i] = j;
    }
    for (int i = 15, j = floatRegs.size(); i >= 1; --i, ++j)
    {
        floatRegs.emplace_back(i);
        floatRegsIndex[i] = j;
    }

    // 记录保留的寄存器
    funcAttr.attr.used_regs.intReservedReg = 4;
    funcAttr.attr.used_regs.floatReservedReg = 16;
    SET_UINT(funcAttr.attr.used_regs.intRegs, 0);
    SET_UINT(funcAttr.attr.used_regs.intRegs, 4);
    for (int i = 13; i < 16; ++i)
        SET_UINT(funcAttr.attr.used_regs.intRegs, i);
    SET_UINT(funcAttr.attr.used_regs.floatRegs, 0);
    SET_UINT(funcAttr.attr.used_regs.floatRegs, 16);

    // 记录当前局部变量的栈偏移
    int varStackOffset = 0;

    // 将分配到栈的信息存储到symAttr并更新栈顶
    auto AllocOnStackMark = [&varStackOffset](SymAttribute &symAttr, int size)
    {
        if (INT_MAX - size < varStackOffset)
            throw std::runtime_error("RegAllocator: variable stack overflow");
        symAttr.attr.store_type = SymAttribute::STACK_VAR;
        symAttr.value = varStackOffset;
        varStackOffset += size;
    };

    // 将寄存器regId加入到函数使用的type类寄存器集
    auto funcUsedRegadd = [this](SymValueType type, int regId)
    {
        if (type == INT)
            SET_UINT(this->funcAttr.attr.used_regs.intRegs, regId);
        else
            SET_UINT(this->funcAttr.attr.used_regs.floatRegs, regId);
    };

    // 为symAttr记录信息：分配到type类型的寄存器regId，并更新函数使用的寄存器集
    auto AllocOnRegMark = [&funcUsedRegadd](SymAttribute &symAttr, SymValueType type, int regId)
    {
        if (type == INT)
            symAttr.attr.store_type = SymAttribute::INT_REG;
        else
            symAttr.attr.store_type = SymAttribute::FLOAT_REG;
        symAttr.value = regId;
        funcUsedRegadd(type, regId);
    };
    

    // 开始为每个变量分配物理寄存器或栈空间
    while (!syms.empty())
    {
        auto symInfo = syms.top();
        syms.pop();

        // 得到适合于这个变量的寄存器集(INT使用通用寄存器，FLOAT使用浮点寄存器)
        auto &regs = *(typeRegMap[symInfo.symValueType]);
        auto regIndex = typeRegIndexMap[symInfo.symValueType];

        auto &attribute = fetchSymAttr(symInfo);

        // 如果sym是通过寄存器传递的参数
        if (symInfo.symType == PARAM)
        {
            if (attribute.attr.store_type != SymAttribute::STACK_PARAM)  // 判断为true代表在寄存器中
            {  
                auto regId = attribute.value;
                auto &reg = regs[regIndex[regId]];

                // 尝试将该参数分配给regId
                // 成功，则分配完成，不需要更改attribute信息，只更新函数使用的寄存器
                if (reg.AllocToSym(*(symInfo.liveRanges)) == true)
                {
                    funcUsedRegadd(symInfo.symValueType, regId);
                    continue;
                }
                // 失败，溢出到栈
                else
                {
                    AllocOnStackMark(attribute, 4);  // 参数始终是4字节
                    continue;
                }
            }    
        }

        // 否则，sym是通过栈传递的参数或局部变量
        bool allocInReg = false;
        for (auto &reg : regs)
        {
            // 如果能找到一个不冲突的寄存器reg，则分配到reg
            if (reg.AllocToSym(*(symInfo.liveRanges)) == true)
            {
                AllocOnRegMark(attribute, symInfo.symValueType, reg.id);
                allocInReg = true;
                break;
            }
        }

        // 如果所有寄存器都冲突，由于是按溢出代价从高到低分配，将当前sym溢出代价最小
        if (allocInReg == false)
        {
            // 如果sym是通过栈传递的参数，则不需移动
            // 局部变量则溢出到栈
            if (symInfo.symType == LOCAL_VAR)
                AllocOnStackMark(attribute, 4);  // 局部变量始终是4字节
        }
    }


    // 接下来为通过局部变量指针引用的，存放在栈上的变量分配空间
    // 目前即数组
    for (auto& [sym, attr] : ptrToArrayOnStack)
    {
        auto siz = sym->value_.GetArrayDescriptor()->GetSizeInByte();
        AllocOnStackMark(attr, siz);
    }

    // 将栈的使用情况记录到函数属性
    auto &varStackSize = funcAttr.value;
    varStackSize = abs(varStackOffset);
}

bool RegAllocator::RegInfo::IsConflict(const std::set<LiveInterval>& symRanges)
{
    auto regIt = occupyRanges.begin(), symIt = symRanges.begin();
    for (; symIt != symRanges.end() && regIt != occupyRanges.end(); )
    {
        if (regIt->second < symIt->first)  ++regIt;
        else if (symIt->second < regIt->first)  ++symIt;
        else  break;
    }
    if (symIt == symRanges.end() || regIt == occupyRanges.end())  return false;
    else  return true;
}

void RegAllocator::RegInfo::AddRanges(const std::set<LiveInterval>& src)
{
    for (auto &e : src)
        occupyRanges.insert(e);
}

bool RegAllocator::RegInfo::AllocToSym(const std::set<LiveInterval> &symRanges)
{
    if (IsConflict(symRanges))
        return false;
    AddRanges(symRanges);
    return true;
}

RegAllocator::RegAllocator(const LiveAnalyzer& liveAnalyzer)
{
    // 得到函数中的局部变量、参数列表
    ContextInit(liveAnalyzer);
    // 线性扫描
    LinearScan(liveAnalyzer);
    // 在symAttrMap中添加函数属性
    if (symAttrMap.emplace((*liveAnalyzer.get_fbegin())->a_, funcAttr).second == false)
        throw std::runtime_error("RegAllocator error: Unable to insert function attribute");
}

SymAttribute RegAllocator::get_SymAttribute(SymPtr sym) 
{ 
    return fetchAttr(sym, symAttrMap);
}

SymAttribute RegAllocator::get_ArrayAttribute(SymPtr arrPtr)
{
    return fetchAttr(arrPtr, ptrToArrayOnStack);
}

SymAttribute RegAllocator::fetchAttr(SymPtr sym, std::unordered_map<SymPtr, SymAttribute>& attrMap)
{
    auto res = attrMap.find(sym);
    if (res == attrMap.end())
        throw std::runtime_error("Sym attribute not found");
    return res->second;
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler