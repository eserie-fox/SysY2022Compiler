#pragma once

#include "ASM/Common.hh"
#include "TAC/ThreeAddressCode.hh"
#include "MacroUtil.hh"
#include <vector>
#include <iostream>
#include <stdexcept>

namespace HaveFunCompiler{
namespace AssemblyBuilder{

class ControlFlowGraph
{
public:

    using TACPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>;

    NONCOPYABLE(ControlFlowGraph)

    // 传入一个函数的三地址码列表，生成函数的控制流图
    // [fbegin, fend)
    // fbegin-> label funcName
    // fend-> fend的后一条
    ControlFlowGraph(TACListPtr FuncTACList);
    ControlFlowGraph(TACList::iterator fbegin, TACList::iterator fend);

    const TACList::iterator get_fbegin() const
    {
        return f_begin;
    }

    const TACList::iterator get_fend() const
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

    size_t get_inDegree(size_t n) const
    {
        return nodes[n].inNodeList.size();
    }

    size_t get_outDegree(size_t n) const
    {
        return nodes[n].outNodeList.size();
    }

    static size_t get_startNode()
    {
        return startNode;
    }

    static size_t get_endNode()
    {
        return endNode;
    }

    size_t get_nodes_number() const
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

    const TACList::iterator& get_node_itr(size_t n) const
    {
        return itrMap[n];
    }

    const std::vector<TACList::iterator>& get_unreachableTACItrList() const
    {
        return unreachableTACItrList;
    }

    void setDfn();
//    void doDfn(size_t &cnt, std::vector<bool> &vis, size_t u);

    inline void eraseNode(size_t u);

    // 测试用
    void printToDot() const;

private:

    struct Node
    {
        TACPtr tac;
        std::vector<size_t> inNodeList, outNodeList;

        // 结点的dfs序
        // 在没有dfs排序前，dfn代表该结点是否被删除（不为0存在，为0被删除）
        size_t dfn;

        Node() { dfn = 1; }
        Node(TACPtr it) : tac(it) { dfn = 1; } 
    };

    std::vector<Node> nodes;
    TACList::iterator f_begin, f_end;
    std::vector<TACList::iterator> unreachableTACItrList;
    std::vector<TACList::iterator> itrMap;  // 保存node下标到itr的映射

    // n1 -> n2
    void link(size_t n1, size_t n2);

    // 创建一个新结点并返回新结点编号
    size_t newNode();
    size_t newNode(TACPtr tac);

    std::string getJmpLabel(TACPtr tac);

    void WarnUnreachable() const;

    void dfsPrintToDot(size_t n, std::vector<bool> &vis, std::ostream &os) const;


    static const size_t startNode, endNode;


public:

    // 由于图采用懒惰删除，外部可使用这些接口直接迭代nodes数组下标
    size_t beginIdx() const
    {
        return 0;
    }

    size_t endIdx() const
    {
        return nodes.size();
    }

    size_t nextIdx(size_t idx) const
    {
        do
        {
            ++idx;
        } while (idx < nodes.size() && nodes[idx].dfn == 0);
        return idx;
    }
};

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler