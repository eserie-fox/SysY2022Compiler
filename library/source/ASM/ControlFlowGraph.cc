#include "ASM/ControlFlowGraph.hh"
#include "TAC/Symbol.hh"
#include <unordered_map>

#include <cstdio>

namespace HaveFunCompiler{
namespace AssemblyBuilder{

ControlFlowGraph::ControlFlowGraph(TACListPtr FuncTACList) : ControlFlowGraph(FuncTACList->begin(), FuncTACList->end()) { }

ControlFlowGraph::ControlFlowGraph(TACList::iterator fbegin, TACList::iterator fend)
{
    using namespace HaveFunCompiler::ThreeAddressCode;

    std::unordered_map<std::string, size_t> labelMap;  // 映射jmp目标对应节点下标
    std::vector<size_t> jmpReloc;  // 第一次扫描时保存需要jmp的指令

    nodes.resize(2);
    nodes[startNode].tac = *fbegin;
    nodes[endNode].tac = *fend;    

    size_t cur = 0;

    for (auto it = fbegin; it != fend; )
    {
        auto nxtIt = it;
        ++nxtIt;
        size_t nxtcur = newNode(*nxtIt);
        
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

void ControlFlowGraph::print()
{
    size_t cur = 0;
    for (auto &e : nodes)
    {
        printf("Node %lu:\n", cur);
        printf("%s\n", e.tac->ToString().c_str());

        printf("inNodes: ");
        for (auto n : e.inNodeList)
            printf("%lu ", n);
        printf("\n");

        printf("outNodes: ");
        for (auto n : e.outNodeList)
            printf("%lu ", n);
        printf("\n");

        printf("\n");
        ++cur;
    }
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler
