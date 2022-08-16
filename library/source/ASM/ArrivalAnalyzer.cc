#include "ASM/ArrivalAnalyzer.hh"
#include "ASM/SymAnalyzer.hh"
#include "TAC/Symbol.hh"
#include <optional>

namespace HaveFunCompiler{
namespace AssemblyBuilder{


ArrivalAnalyzer::ArrivalAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph) : 
    DataFlowAnalyzerForward<ArrivalInfo>(controlFlowGraph)
{
    symAnalyzer = std::make_shared<SymAnalyzer>(cfg);
    symAnalyzer->analyze();
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
        useDefChain[sym][i];   // 确保映射中sym存在，即使它的定值链为空
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
    if (_in[x].isUnvSet)   // in是全集
        _in[x] = _out[y];
    else if (_out[y].isUnvSet)  // out[pre]是全集
        return;
    else  // 都不是全集
    {
        auto &exps = _in[x].exps;
        for (auto it = exps.begin(); it != exps.end(); )
        {
            // it -> <expid, 上一个计算exp的点下标>
            // 求交集
            // BUG: erase的参数迭代器会被非法化，使用it = erase(it)
            if (_out[y].exps.count(it->first) == 0)
                it = exps.erase(it);
            else
                ++it;
        }
    }
}

void ArrivalExprAnalyzer::transFunc(size_t u)
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
    std::optional<size_t> gen;
    std::vector<size_t> *kill = nullptr;
    
    auto getKill = [this](SymbolPtr sym) -> std::vector<size_t>* 
    {
        auto res = symExpLink.find(sym);
        if (res != symExpLink.end())
            return &res->second;
        return nullptr;
    };

    // 构造gen集合
    // 如果是二元运算语句
    if (isBin(tac))
    {
        // 排除常量表达式
        if (!tac->b_->IsLiteral() && !tac->c_->IsLiteral())
        {
            ExprInfo expr(tac->b_, tac->c_, (ExprInfo::Op)tac->operation_);
            auto [itr, notExist] = exprMap.emplace(expr, idxMap.size());
            // 如果表达式集合中还没有这条语句的表达式信息，则添加映射
            if (notExist)
            {
                symExpLink[tac->b_].push_back(idxMap.size());
                symExpLink[tac->c_].push_back(idxMap.size());
                idxMap.push_back(expr);
            }
            gen = itr->second;
        }
    }

    // 如果是一元运算语句
    else if (isUn(tac))
    {
        if (!tac->b_->IsLiteral())
        {
            ExprInfo expr(tac->b_, (ExprInfo::Op)tac->operation_);
            auto [itr, notExist] = exprMap.emplace(expr, idxMap.size());
            if (notExist)
            {
                symExpLink[tac->b_].push_back(idxMap.size());
                idxMap.push_back(expr);
            }
            gen = itr->second;
        }
    }

    // 如果是赋值语句
    else if (isAssign(tac))
    {
        // 若赋值右边是数组访问
        if (tac->b_->value_.Type() == SymbolValue::ValueType::Array)
        {
            auto base = tac->b_->value_.GetArrayDescriptor()->base_addr.lock();
            auto offset = tac->b_->value_.GetArrayDescriptor()->base_offset;

            ExprInfo expr(base, offset, ExprInfo::Op::Access);
            auto [itr, notExist] = exprMap.emplace(expr, idxMap.size());
            if (notExist)
            {
                symExpLink[base].push_back(idxMap.size());
                symExpLink[offset].push_back(idxMap.size());
                idxMap.push_back(expr);
            }
            gen = itr->second;
        }
        // 否则，赋值右边是变量或字面常量
        else
        {
            if (!tac->b_->IsLiteral())  // 仍然排除字面常量
            {
                ExprInfo expr(tac->b_);
                auto [itr, notExist] = exprMap.emplace(expr, idxMap.size());
                if (notExist)
                {
                    symExpLink[tac->b_].push_back(idxMap.size());
                    idxMap.push_back(expr);
                }
                gen = itr->second;
            }

            // 此时赋值左边可能是变量或数组访问，如果是数组访问，则无定值而有副作用
            // 如果左边是数组访问，kill掉数组，不管下标
            if (tac->a_->value_.Type() == SymbolValue::ValueType::Array)
            {
                auto base = tac->a_->value_.GetArrayDescriptor()->base_addr.lock();
                kill = getKill(base);
            }
        }
    }

    // 如果有定值，则杀死与定值变量相关的表达式
    auto defSym = tac->getDefineSym();
    if (defSym)
        kill = getKill(defSym);
    // 没有定值，但如果有函数调用传入数组，考虑副作用，杀死与数组相关的表达式
    else if (tac->operation_ == TACOperationType::ArgumentAddress)
    {
        auto base = tac->a_->value_.GetArrayDescriptor()->base_addr.lock();
        kill = getKill(base);
    }

    // out = gen ∪ (in - kill)
    // gen = expr - kill
    _out[u] = _in[u];
    _out[u].isUnvSet = 0;
    if (gen.has_value())
        _out[u].exps[gen.value()] = u;   // 更新表达式的产生点
    if (kill)
        for (auto id : *kill)
            _out[u].exps.erase(id);
}

}
}