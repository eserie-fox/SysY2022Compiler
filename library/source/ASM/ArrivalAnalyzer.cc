#include "ASM/ArrivalAnalyzer.hh"
#include "ASM/SymAnalyzer.hh"
#include "TAC/Symbol.hh"
#include <optional>

namespace HaveFunCompiler{
namespace AssemblyBuilder{


ArrivalAnalyzer::ArrivalAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph, std::shared_ptr<const SymAnalyzer> _symAnalyzer) : 
    DataFlowAnalyzerForward<ArrivalInfo>(controlFlowGraph), symAnalyzer(_symAnalyzer)
{
}

// 到达定值信息结点间传递
// 定值到达结点u的出口，则到达它每一个后继的入口
// 框架调用时, x为y的后继，信息从y传递到x
void ArrivalAnalyzer::transOp(size_t x, size_t y)
{
    for (auto &e : _out[y])
        _in[x].insert(e);
}

// 到达定值信息结点内传递
// out = gen ∪ (in - kill)
// gen = 若当前有定值，则为当前结点id
// kill = 所有其他对当前变量定值的结点id
void ArrivalAnalyzer::transFunc(size_t u)
{
    _out[u] = _in[u];
    auto def = cfg->get_node_tac(u)->getDefineSym();
    if (def)
    {
        auto defSet = symAnalyzer->getSymDefPoints(def);
        for (auto n : defSet)
            _out[u].erase(n);
        _out[u].emplace(u);
    }
}

void ArrivalAnalyzer::updateUseDefChain()
{
    FOR_EACH_NODE(i, cfg)
    {
        auto tac = cfg->get_node_tac(i);
        auto& arrivalDefs = getIn(i);
        auto useSyms = tac->getUseSym();
        
        for (auto sym : useSyms)
        {
        auto &symDefs = symAnalyzer->getSymDefPoints(sym);
        for (auto d : symDefs)
            if (arrivalDefs.count(d))
            useDefChain[sym][i].insert(d);
        }
    }
}

bool ExprInfo::operator==(const ExprInfo& o) const
{
    // 二元运算符是否满足交换律
    auto exchangable = [](Op op)
    {
        static std::unordered_set<Op> exchgable = {
            Op::Add, Op::Mul, Op::Equal, Op::NotEqual, Op::LogicAnd, Op::LogicOr
        };
        return exchgable.count(op);
    };

    if (type != o.type)  return false;
    switch (type)
    {
    case BIN:
        if (op != o.op)  return false;
        if (exchangable(op))
            return (a == o.a && b == o.b) || (a == o.b && b == o.a);
        else
            return a == o.a && b == o.b;
        break;
    
    case UN:
        return a == o.a && op == o.op;
        break;

    case VAR:
        return a == o.a;
        break;

    default:
        return false;
        break;
    }
}

ArrivalExprAnalyzer::ArrivalExprAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph) : 
    DataFlowAnalyzerForward<ArrivalExprInfo>(controlFlowGraph)
{
    auto start = cfg->get_startNode();
    _in[start].isUnvSet = 0;   // 只有起始点的in不初始化为全集
}

void ArrivalExprAnalyzer::transOp(size_t x, size_t y)
{
    if (_in[x].isUnvSet && _out[y].isUnvSet)  // 两个全集相交
        return;
    else if (_in[x].isUnvSet)   // in是全集
        _in[x] = _out[y];
    else if (_out[y].isUnvSet)  // out[pre]是全集
        return;
    else
    {
        auto &exps = _in[x].exps;
        for (auto it = exps.begin(); it != exps.end(); ++it)
        {
            // it -> <expIt, 上一个计算exp的点下标>
            // 求交集
            if (_out[y].exps.count((*it).first))
                exps.erase(it);
        }
    }
}

[[maybe_unused]] void ArrivalExprAnalyzer::transFunc(size_t u)
{
    auto isBin = [](TACPtr tac)
    {
        return tac->operation_ >= TACOperationType::Add && tac->operation_ <= TACOperationType::LogicOr;
    };

    auto isUn = [](TACPtr tac)
    {
        return (tac->operation_ >= TACOperationType::UnaryMinus && tac->operation_ <= TACOperationType::UnaryPositive) ||
               (tac->operation_ == TACOperationType::FloatToInt || tac->operation_ == TACOperationType::IntToFloat);
    };

    auto isAssign = [](TACPtr tac) { return tac->operation_ == TACOperationType::Assign; };

    auto tac = cfg->get_node_tac(u);
    std::optional<ExprItr> gen;
    std::vector<ExprItr> *kill = nullptr;
    
    // 构造gen和kill集合
    if (isBin(tac))
    {
        // 排除常量表达式
        if (!tac->b_->IsLiteral() && !tac->c_->IsLiteral())
        {
            auto res = exprSet.insert(ExprInfo(tac->b_, tac->c_, (ExprInfo::Op)tac->operation_));
            // 如果表达式集合中还没有这条语句的表达式信息，则添加变量映射
            if (res.second)
            {
                symExpLink[tac->b_].push_back(res.first);
                symExpLink[tac->c_].push_back(res.first);
            }
            gen = res.first;
        }

        auto killLs = symExpLink.find(tac->a_);
        if (killLs != symExpLink.end())
            kill = &(killLs->second);
    }
    else if (isUn(tac))
    {
        if (!tac->b_->IsLiteral())
        {
            auto res = exprSet.insert(ExprInfo(tac->b_, (ExprInfo::Op)tac->operation_));
            if (res.second)
                symExpLink[tac->b_].push_back(res.first);
            gen = res.first;
        }

        auto killLs = symExpLink.find(tac->a_);
        if (killLs != symExpLink.end())
            kill = &(killLs->second);
    }
    else if (isAssign(tac))
    {
        // 若赋值右边是数组访问
        if (tac->b_->value_.Type() == SymbolValue::ValueType::Array)
        {
            auto base = tac->b_->value_.GetArrayDescriptor()->base_addr.lock();
            auto offset = tac->b_->value_.GetArrayDescriptor()->base_offset;

            auto res = exprSet.insert(ExprInfo(base, offset, ExprInfo::Op::Access));
            if (res.second)
            {
                symExpLink[base].push_back(res.first);
                symExpLink[offset].push_back(res.first);
            }
            gen = res.first;
        }
        // 否则，赋值右边是变量或字面常量
        else
        {
            if (!tac->b_->IsLiteral())
            {
                auto res = exprSet.insert(ExprInfo(tac->b_));
                
            }
        }
    }
}

}
}