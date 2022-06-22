#pragma once

#include "ASM/Common.hh"
#include "TAC/ThreeAddressCode.hh"
#include "MacroUtil.hh"
#include <vector>
#include <stdexcept>

namespace HaveFunCompiler{
namespace AssemblyBuilder{

class ControlFlowGraph
{
public:

    using TACPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>;

    NONCOPYABLE(ControlFlowGraph)

    // 传入一个函数的三地址码列表，生成函数的控制流图
    ControlFlowGraph(TACListPtr FuncTACList);
    ControlFlowGraph(TACList::iterator fbegin, TACList::iterator fend);

    const TACList::iterator get_fbegin()
    {
        return f_begin;
    }

    const TACList::iterator get_fend()
    {
        return f_end;
    }

    const std::vector<size_t>& get_inNodeList(size_t n) const
    {
        if (n >= nodes.size())
            throw std::out_of_range("cfg get index out of range: get_inNodeList");
        return nodes[n].inNodeList;
    }

    const std::vector<size_t>& get_outNodeList(size_t n) const
    {
        if (n >= nodes.size())
            throw std::out_of_range("cfg get index out of range: get_outNodeList");
        return nodes[n].outNodeList;
    }

    static size_t get_startNode()
    {
        return startNode;
    }

    static size_t get_endNode()
    {
        return endNode;
    }

    size_t get_nodes_number()
    {
        return nodes.size();
    }

    TACPtr get_node_tac(size_t n) const
    {
        if (n >= nodes.size())
            throw std::out_of_range("cfg get index out of range: get_node_tac");
        return nodes[n].tac;
    }

    size_t get_node_dfn(size_t n) const
    {
        if (n >= nodes.size())
            throw std::out_of_range("cfg get index out of range: get_node_dfn");
        return nodes[n].dfn;
    }

    // 测试用
    void print();

private:

    struct Node
    {
        TACPtr tac;
        std::vector<size_t> inNodeList, outNodeList;

        size_t dfn;  // 结点的dfs序

        Node() {}
        Node(TACPtr it) : tac(it) {} 
    };

    std::vector<Node> nodes;
    TACList::iterator f_begin, f_end;

    static const size_t startNode = 0, endNode = 1;

    // n1 -> n2
    void link(size_t n1, size_t n2);

    // 创建一个新结点并返回新结点编号
    size_t newNode();
    size_t newNode(TACPtr tac);

    std::string getJmpLabel(TACPtr tac);

    void getDfn();
    void doDfn(size_t &cnt, std::vector<bool> &vis, size_t u);
};

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler