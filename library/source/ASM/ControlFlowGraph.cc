#include "ASM/ControlFlowGraph.hh"
#include "TAC/Symbol.hh"
#include <unordered_map>
#include <string>
#include <fstream>

namespace HaveFunCompiler{
namespace AssemblyBuilder{

const size_t ControlFlowGraph::startNode = 0;
const size_t ControlFlowGraph::endNode = 1;

ControlFlowGraph::ControlFlowGraph(TACListPtr FuncTACList) : ControlFlowGraph(FuncTACList->begin(), FuncTACList->end()) { }

ControlFlowGraph::ControlFlowGraph(TACList::iterator fbegin, TACList::iterator fend)
{
    using namespace HaveFunCompiler::ThreeAddressCode;

    // 合法性检查
    if (fbegin == fend)
        throw std::runtime_error("ControlFlowGraph error: empty tac list");
    --fend;
    if ((*fbegin)->operation_ != TACOperationType::Label || (*fend)->operation_ != TACOperationType::FunctionEnd)
        throw std::runtime_error("ControlFLowGraph error: unrecognized function TAClist");

    f_begin = fbegin;
    f_end = fend;

    std::unordered_map<std::string, size_t> labelMap;  // 映射jmp目标对应节点下标
    std::vector<size_t> jmpReloc;  // 第一次扫描时保存需要jmp的指令

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
    
    setDfn();

    // 检查不可达代码并将不可达信息输出
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        if (nodes[i].dfn == 0)
            unreachableTACItrList.push_back(itrMap[i]);
    }
//    WarnUnreachable();
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


// void ControlFlowGraph::doDfn(size_t &cnt, std::vector<bool> &vis, size_t u)
// {
//     if (vis[u])
//         return;
//     nodes[u].dfn = ++cnt;
//     vis[u] = 1;
//     for (auto v : nodes[u].outNodeList)
//         doDfn(cnt, vis, v);
// }

void ControlFlowGraph::setDfn()
{
    size_t cnt = 0;
    std::vector<bool> vis(nodes.size(), 0);

    std::vector<std::pair<size_t, size_t>> st;  // dfs状态[结点号，将要访问的邻接结点在邻接表的下标]
    vis[startNode] = 1;
    nodes[startNode].dfn = ++cnt;
    st.emplace_back(startNode, 0);

    while (true)
    {
        auto& [u, idx] = st.back();
        if (idx == nodes[u].outNodeList.size())
        {
            st.pop_back();
            if (st.empty())
                break;
            ++st.back().second;
        }
        else
        {
            auto v = nodes[u].outNodeList[idx];
            if (!vis[v])
            {
                vis[v] = 1;
                nodes[v].dfn = ++cnt;
                st.emplace_back(v, 0);
            }
            else
                ++idx;
        }
    }

//    doDfn(cnt, vis, startNode);
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

void ControlFlowGraph::printToDot() const
{
    // for (size_t i = 0; i < nodes.size(); ++i)
    // {
    //     if (nodes[i].dfn == 0)
    //         continue;

    //     printf("Node %lu:\n", i);
    //     printf("%s\n", nodes[i].tac->ToString().c_str());

    //     printf("inNodes: ");
    //     for (auto n : nodes[i].inNodeList)
    //         printf("%lu ", n);
    //     printf("\n");

    //     printf("outNodes: ");
    //     for (auto n : nodes[i].outNodeList)
    //         printf("%lu ", n);
    //     printf("\n");

    //     printf("dfn: %lu\n", nodes[i].dfn);

    //     printf("\n");
    // }
    std::ofstream os("cfg.dot");
    os << "digraph CFG {\n";

    std::vector<bool> vis(nodes.size(), 0);
    dfsPrintToDot(startNode, vis, os);
    os << "}";
}

void ControlFlowGraph::dfsPrintToDot(size_t n, std::vector<bool> &vis, std::ostream &os) const
{
    auto nodeToDotLabel = [](const Node &node)
    {
        std::string s = std::to_string(node.dfn);
        s += "\\n";
        s += node.tac->ToString();
        return s;
    };
    vis[n] = true;
    os << nodes[n].dfn << " [label=\"" << nodeToDotLabel(nodes[n]) << "\"];\n";
    for (auto u : nodes[n].outNodeList)
    {
        os << nodes[n].dfn << " -> " << nodes[u].dfn << " ;\n";
        if (!vis[u])
            dfsPrintToDot(u, vis, os);
    }
}

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler
