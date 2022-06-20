#pragma once

#include "ASM/Common.hh"
#include "TAC/ThreeAddressCode.hh"
#include "MacroUtil.hh"
#include <vector>

namespace HaveFunCompiler{
namespace AssemblyBuilder{

class ControlFlowGraph
{
private:
    
    using TACPtr = std::shared_ptr<HaveFunCompiler::ThreeAddressCode::ThreeAddressCode>;

    struct Node
    {
        TACPtr tac;
        std::vector<size_t> inNodeList, outNodeList;

        Node() {}
        Node(TACPtr it) : tac(it) {} 
    };

    std::vector<Node> nodes;

    static const size_t startNode = 0, endNode = 1;

    // n1 -> n2
    void link(size_t n1, size_t n2);

    // 创建一个新结点并返回新结点编号
    size_t newNode();
    size_t newNode(TACPtr tac);

    std::string getJmpLabel(TACPtr tac);

public:
    NONCOPYABLE(ControlFlowGraph)

    // 传入一个函数的三地址码列表，生成函数的控制流图
    ControlFlowGraph(TACListPtr FuncTACList);
    ControlFlowGraph(TACList::iterator fbegin, TACList::iterator fend);

    // 测试用
    void print();
};

}  // namespace AssemblyBuilder
}  // namespace HaveFunCompiler