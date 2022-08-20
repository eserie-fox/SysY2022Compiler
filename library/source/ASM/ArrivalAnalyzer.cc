#include "ASM/ArrivalAnalyzer.hh"
#include "ASM/SymAnalyzer.hh"
#include "ASM/LiveAnalyzer.hh"
#include "TAC/Symbol.hh"
#include <cassert>
#include <vector>
#include <queue>

namespace HaveFunCompiler{
namespace AssemblyBuilder{


ArrivalAnalyzer::ArrivalAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph) : 
    DataFlowAnalyzerForward<ArrivalInfo>(controlFlowGraph)
{
    symAnalyzer = std::make_shared<SymAnalyzer>(cfg);
    symAnalyzer->analyze();
    liveAnalyzer = std::make_shared<LiveAnalyzer>(cfg, symAnalyzer);
    liveAnalyzer->analyze();
    
    FOR_EACH_NODE(n, cfg)
    {
        auto tac = cfg->get_node_tac(n);
        auto defSym = tac->getDefineSym();
        if (defSym)
        {
            if (defSym->IsGlobal())
                defIDsOfGlobal.push_back(idDefMap.size());
            defIdMap[n] = idDefMap.size();
            idDefMap.push_back(n);
        }
        else if (tac->operation_ == TACOperationType::Parameter)  // 参数声明需要算作定值
        {
            defIdMap[n] = idDefMap.size();
            idDefMap.push_back(n);
        }
    }

    if (idDefMap.size() != 0)
        initInfo = ArrivalInfo(idDefMap.size());
    FOR_EACH_NODE(n, cfg)
    {
        _in[n] = initInfo;
        _out[n] = initInfo;
    }
}

// 到达定值信息结点间传递
// 定值到达结点u的出口，则到达它每一个后继的入口
// 框架调用时, x为y的后继，信息从y传递到x
void ArrivalAnalyzer::transOp(size_t x, size_t y)
{
    _in[x] |= _out[y];
}

// 到达定值信息结点内传递
// out = gen ∪ (in - kill)
// gen = 若当前有定值，则为当前结点id
// kill = 所有其他对当前变量定值的结点id
void ArrivalAnalyzer::transFunc(size_t u)
{
    auto isDecl = [](TACOperationType op)
    {
        switch (op)
        {
        case TACOperationType::Variable:
        case TACOperationType::Constant:
        return true;
        break;
        
        default:
        return false;
        break;
        }
    };

    _out[u] = _in[u];
    auto tac = cfg->get_node_tac(u);
    auto def = tac->getDefineSym();
    if (def && !isDecl(tac->operation_))
    {
        // 杀死定值变量的所有其他定值
        auto defSet = symAnalyzer->getSymDefPoints(def);
        std::vector<size_t> defVec;
        for (auto n : defSet)
            defVec.push_back(defIdMap[n]);
        _out[u].reset(defVec);
        
        // 仅当当前变量在出口仍然活跃，才需要继续传递定值
        if (liveAnalyzer->isOutLive(def, u))
            _out[u].set(defIdMap[u]);
    }

    // 如果当前tac有副作用(call)，删除全部全局变量的定值
    if (tac->operation_ == TACOperationType::Call)
        _out[u].reset(defIDsOfGlobal);
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
                if (arrivalDefs.test(defIdMap[d]))
                    useDefChain[sym][i].insert(d);
        }
    }
}

bool ArrivalAnalyzer::isReachableDef(size_t def, size_t node) const
{
    auto &inDefIds = getIn(node);
    return inDefIds.test(defIdMap.at(def));
}

std::unordered_set<size_t> ArrivalAnalyzer::getReachableDefs(size_t node) const
{
    auto defIds = getIn(node).get_elements();
    std::unordered_set<size_t> res;
    for (auto id : defIds)
        res.emplace(idDefMap[id]);
    return res;
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


CommExpAnalyzer::CommExpAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph) : cfg(controlFlowGraph)
{
}

std::optional<ExprInfo> CommExpAnalyzer::getRhs(size_t u)
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
    std::optional<ExprInfo> ret = std::nullopt;

    // 如果是二元运算语句
    // 排除常量表达式
    if (isBin(tac))
    {
        if (!tac->b_->IsLiteral() && !tac->c_->IsLiteral())
            ret = std::make_optional<ExprInfo>(tac->b_, tac->c_, (ExprInfo::Op)tac->operation_);
    }

    // 如果是一元运算语句
    else if (isUn(tac))
    {
        if (!tac->b_->IsLiteral())
            ret = std::make_optional<ExprInfo>(tac->b_, (ExprInfo::Op)tac->operation_);
    }

    // 如果是赋值语句
    else if (isAssign(tac))
    {
        // 赋值右边是数组访问，才记录为表达式
        if (tac->b_->value_.Type() == SymbolValue::ValueType::Array)
        {
            auto base = tac->b_->value_.GetArrayDescriptor()->base_addr.lock();
            auto offset = tac->b_->value_.GetArrayDescriptor()->base_offset;
            ret = std::make_optional<ExprInfo>(base, offset, ExprInfo::Op::Access);
        }
    } 

    return ret;
}

void CommExpAnalyzer::analyze()
{
    // 获得所有公共的表达式并映射为id
    std::unordered_set<ExprInfo> allExp;
    FOR_EACH_NODE(n, cfg)
    {
        auto expContainer = getRhs(n);
        if (expContainer.has_value())
        {
            auto exp = expContainer.value();

            // 已经存在，说明是重复的表达式
            if (allExp.count(exp) != 0)
            {
                // 如果没有未其分配编号，则分配一个
                if (!CommExp2Id.count(exp))
                {
                    auto id = id2CommExp.size();
                    CommExp2Id.emplace(exp, id);
                    id2CommExp.push_back(exp);
                    symExpLink[exp.a].push_back(id);
                    if (exp.b)
                        symExpLink[exp.b].push_back(id);
                }
            }

            // 否则，加入allExp
            else
                allExp.insert(exp);
        }

        // 得到函数中出现的全局变量，后续分析使用
        auto tac = cfg->get_node_tac(n);
        auto defSym = tac->getDefineSym();
        if (defSym && defSym->IsGlobal())
            globalSyms.insert(defSym);
        auto useSyms = tac->getUseSym();
        for (auto sym : useSyms)
            if (sym->IsGlobal())
                globalSyms.insert(sym);
    }

    // 没有重复的表达式，直接返回
    if (id2CommExp.size() == 0)
        return;

    std::vector<bool> vis(cfg->get_nodes_number(), 0);

    // 记录每一个结点的可用公共表达式
    std::unordered_map<size_t, RopeContainer<size_t>> usableExps;
    // 记录每一个结点，可用公共表达式的定值点
    // RopeArr[i]，表示在结点处，公共表达式i的到达定值点（仅在结点可用时信息有意义）
    std::unordered_map<size_t, RopeArr> expDef;

    // rope的初始值
    RopeContainer<size_t> initUsableExps(id2CommExp.size());
    RopeArr initExpDef(id2CommExp.size(), 0);

    // 每次从一个点开始，遍历它及其下属单链，在这个范围内进行公共子表达式删除
    FOR_EACH_NODE(n, cfg)
    {
        if (vis[n])
            continue;
        
        std::queue<size_t> q;
        q.push(n);
        usableExps[n] = initUsableExps;
        expDef[n] = initExpDef;

        while (!q.empty())
        {
            auto u = q.front();
            q.pop();
            if (vis[u])  
                continue;
            vis[u] = 1;

            // 得到u处出现的表达式
            auto expOptional = getRhs(u);
            if (expOptional.has_value())
            {
                auto exp = expOptional.value();

                // 如果这个表达式是公共表达式
                if (CommExp2Id.count(exp) != 0)
                {
                    auto expId = CommExp2Id[exp];
                    // 如果该表达式在u已经可用
                    if (usableExps[u].test(expId))
                    {
                        // 说明此处的exp是可以删除的公共表达式，在exp的定值点记录
                        // exp的定值点在expDef[u][expId]中
                        expUseChain[expDef[u][expId]].push_back(u);
                    }

                    // 否则，标记表达式exp在u定值，且接下来可用
                    else
                    {
                        usableExps[u].set(expId);
                        expDef[u].replace(expId, u);
                    }
                }
            }

            // 计算这条语句杀死的表达式
            // 分四种情况：
            // 1. 有定值，杀死与定值变量相关的表达式
            // 2. 如果赋值给数组，保守起见，杀死该数组相关的表达式
            // 3. 如果使用数组传参，同样杀死该数组相关的表达式
            // 4. 如果是函数调用，杀死所有全局变量相关的表达式
            auto tac = cfg->get_node_tac(u);  

            // 如果有定值
            auto defSym = tac->getDefineSym();
            if (defSym)
                usableExps[u].reset(symExpLink[defSym]);

            // 如果赋值给数组或使用数组传参
            else if ((tac->operation_ == TACOperationType::Assign && tac->a_->value_.Type() == SymbolValue::ValueType::Array) ||
                     (tac->operation_ == TACOperationType::ArgumentAddress))            
            {
                auto arr = tac->a_->value_.GetArrayDescriptor()->base_addr.lock();
                usableExps[u].reset(symExpLink[arr]);
            }

            // 如果是函数调用
            else if (tac->operation_ == TACOperationType::Call)
            {
                std::vector<size_t> resetVec;
                for (auto sym : globalSyms)
                {
                    auto &symLinks = symExpLink[sym];
                    resetVec.insert(resetVec.end(), symLinks.begin(), symLinks.end());
                }
                usableExps[u].reset(resetVec);
            }

            for (auto v : cfg->get_outNodeList(u))
            {
                if (!vis[v] && cfg->get_inDegree(v) == 1)
                {
                    q.push(v);
                    // 可用公共表达式的信息继续向下传递
                    usableExps[v] = usableExps[u];
                    expDef[v] = expDef[u];
                }
            }
        }
    }
}

}
}