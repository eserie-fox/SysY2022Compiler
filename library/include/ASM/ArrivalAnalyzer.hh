#pragma once

#include "ASM/DataFlowAnalyzer.hh"
#include "TAC/ThreeAddressCode.hh"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace HaveFunCompiler{
namespace AssemblyBuilder{

class ControlFlowGraph;
class SymAnalyzer;
using namespace ThreeAddressCode;

// 到达定值分析器
using ArrivalInfo = std::unordered_set<size_t>;
class ArrivalAnalyzer : public DataFlowAnalyzerForward<ArrivalInfo>
{
public:
    ArrivalAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph);
    NONCOPYABLE(ArrivalAnalyzer)
    
    using UseDefChain_T = std::unordered_map<SymbolPtr, std::unordered_map<size_t, std::unordered_set<size_t>>>;

    void updateUseDefChain();
    const UseDefChain_T& get_useDefChain() const { return useDefChain; }
    // const std::unordered_set<size_t>& get_symDefs(SymbolPtr sym, size_t u);

private:
    void transOp(size_t x, size_t y) override;
    void transFunc(size_t u) override;

    std::shared_ptr<SymAnalyzer> symAnalyzer;

    UseDefChain_T useDefChain;
};

struct ExprInfo
{
    enum Type {BIN, UN, VAR} type;
    enum Op {
        Add=1,Sub,Mul,Div,Mod,Equal,NotEqual,LessThan,LessOrEqual,GreaterThan,
        GreaterOrEqual,LogicAnd,LogicOr,UnaryMinus,UnaryNot,UnaryPositive,
        FloatToInt=30,IntToFloat,
        Access
    };

    SymbolPtr a, b;
    Op op;

    ExprInfo(SymbolPtr a_) : type(VAR), a(a_) {}
    ExprInfo(SymbolPtr a_, Op op_) : type(UN), a(a_), op(op_) {}
    ExprInfo(SymbolPtr a_, SymbolPtr b_, Op op_) : type(BIN), a(a_), b(b_), op(op_) {}

    bool operator==(const ExprInfo& o) const;
};

}
}

namespace std
{
    using namespace HaveFunCompiler::AssemblyBuilder;
    template<>
    struct hash<ExprInfo>
    {
        size_t operator()(const ExprInfo& o) const
        {
            if (o.type == ExprInfo::BIN)
                return hash<SymbolPtr>()(o.a) ^ hash<SymbolPtr>()(o.b) ^ (size_t)o.op;
            else if (o.type == ExprInfo::UN)
                return hash<SymbolPtr>()(o.a) ^ (size_t)o.op;
            else
                return hash<SymbolPtr>()(o.a);
        }
    };
}

namespace HaveFunCompiler{
namespace AssemblyBuilder{

using ExprSet = std::unordered_set<ExprInfo>;
using ExprItr = std::unordered_set<ExprInfo>::iterator;

struct ArrivalExprInfo
{
    std::unordered_map<size_t, size_t> exps;
    char isUnvSet;   // 标志是否为全集

    ArrivalExprInfo() : isUnvSet(1) {}

    bool operator==(const ArrivalExprInfo &o) const
    {
        return exps == o.exps && isUnvSet == o.isUnvSet;
    }

    bool operator!=(const ArrivalExprInfo &o) const
    {
        return !(this->operator==(o));
    }
};

// 可用表达式分析器
class ArrivalExprAnalyzer : public DataFlowAnalyzerForward<ArrivalExprInfo>
{
public:
    ArrivalExprAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph);
    NONCOPYABLE(ArrivalExprAnalyzer)

    size_t getIdOfExpr(ExprInfo expr) const
    {
        return exprMap.at(expr);
    }

private:
    void transOp(size_t x, size_t y) override;
    void transFunc(size_t u) override;

    std::unordered_map<ExprInfo, size_t> exprMap;
    std::vector<ExprInfo> idxMap;
    std::unordered_map<SymbolPtr, std::vector<size_t>> symExpLink;  // sym映射到包含它的表达式
};

}
}