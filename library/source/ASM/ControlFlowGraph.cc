#include "ASM/ControlFlowGraph.hh"
#include "TAC/Symbol.hh"
#include <unordered_map>
#include <iostream>

namespace HaveFunCompiler{
namespace AssemblyBuilder{

ControlFlowGraph::ControlFlowGraph(TACListPtr FuncTACList) : ControlFlowGraph(FuncTACList->begin(), FuncTACList->end()) { }

ControlFlowGraph::ControlFlowGraph(TACList::iterator fbegin, TACList::iterator fend) : f_begin(fbegin), f_end(fend)
{
    using namespace HaveFunCompiler::ThreeAddressCode;

    std::unordered_map<std::string, size_t> labelMap;  // 映射jmp目标对应节点下标
    std::vector<size_t> jmpReloc;  // 第一次扫描时保存需要jmp的指令
    std::vector<TACList::iterator> itrMap;  // 保存node下标到itr的映射，扫描不可达点时使用

    // nodes[0]->fbegin(label)
    // nodes[1]->fend
    // 接下来nodes[i]依次对应fbegin和fend中间的tac
    nodes.resize(2);
    nodes[startNode].tac = *fbegin;
    nodes[endNode].tac = *fend;   
    itrMap.push_back(fbegin), itrMap.push_back(fend);

    size_t cur = 0;
    for (auto it = fbegin; it != fend; )
    {
        auto nxtIt = it;
        ++nxtIt;
        size_t nxtcur = nxtIt == fend ? endNode : newNode(*nxtIt);
        itrMap.push_back(nxtIt);

        switch ((*it)->operation_)
        {
        case TACOperationType::Goto:
            jmpReloc.push_back(cur);
            break;
        
        case TACOperationType::IfZero:
            jmpReloc.push_back(cur);
            link(cur, nxtcur);
            break;

        case TACOperationType::Return:
            link(cur, endNode);
            break;

        case TACOperationType::Label:
            link(cur, nxtcur);
            labelMap.emplace(getJmpLabel(*it), cur);
            break;

        default:
            link(cur, nxtcur);
            break;
        }

        cur = nxtcur;
        it = nxtIt;
    }

    for (auto e : jmpReloc)
        link(e, labelMap[getJmpLabel(nodes[e].tac)]);
    
    getDfn();

    // 检查不可达代码并将不可达信息输出
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        if (nodes[i].dfn == 0)
            unreachableTACItrList.push_back(itrMap[i]);
    }
    WarnUnreachable();
}

void ControlFlowGraph::link(size_t n1, size_t n2)
{
    nodes[n1].outNodeList.push_back(n2);
    nodes[n2].inNodeList.push_back(n1);
}

size_t ControlFlowGraph::newNode()
{
    nodes.emplace_back();
    return nodes.size() - 1;
}

size_t ControlFlowGraph::newNode(TACPtr tac)
{
    nodes.emplace_back(tac);
    return nodes.size() - 1;
}


inline std::string ControlFlowGraph::getJmpLabel(TACPtr tac)
{
    return tac->a_->get_name();
}


void ControlFlowGraph::doDfn(size_t &cnt, std::vector<bool> &vis, size_t u)
{
    if (vis[u])
        return;
    nodes[u].dfn = ++cnt;
    vis[u] = 1;
    for (auto v : nodes[u].outNodeList)
        doDfn(cnt, vis, v);
}

void ControlFlowGraph::getDfn()
{
    size_t cnt = 0;
    std::vector<bool> vis(nodes.size(), 0);
    doDfn(cnt, vis, startNode);
}

void ControlFlowGraph::print() const
{
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        if (nodes[i].dfn == 0)
            continue;

        printf("Node %lu:\n", i);
        printf("%s\n", nodes[i].tac->ToString().c_str());

        printf("inNodes: ");
        for (auto n : nodes[i].inNodeList)
            printf("%lu ", n);
        printf("\n");

        printf("outNodes: ");
        for (auto n : nodes[i].outNodeList)
            printf("%lu ", n);
        printf("\n");

        printf("dfn: %lu\n", nodes[i].dfn);

        printf("\n");
    }
}

void ControlFlowGraph::WarnUnreachable() const
{
    if (!unreachableTACItrList.empty())
    {
        std::cerr << "WARNING: function \"" << (*f_begin)->a_->get_name() << 
            "\" contains unreachable statements:\n";
        for (auto itr : unreachableTACItrList)
            std::cerr << (*itr)->ToString() << '\n';
        std::cerr << '\n';
    }
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler
