#include "ASM/SymAnalyzer.hh"
#include "ASM/ControlFlowGraph.hh"
#include <queue>

namespace HaveFunCompiler{
namespace AssemblyBuilder{


SymAnalyzer::SymAnalyzer(std::shared_ptr<const ControlFlowGraph> controlFlowGraph) : cfg(controlFlowGraph)
{  }

void SymAnalyzer::analyze()
{
    std::vector<bool> vis(cfg->get_nodes_number(), false);
    std::queue<size_t> q;
    q.push(cfg->get_startNode());
    while (!q.empty())
    {
        size_t n = q.front();
        q.pop();

        if (vis[n])  continue;
        vis[n] = 1;

        auto tac = cfg->get_node_tac(n);

        // 定值变量处理：加入函数中出现的变量集合，加入该变量的定值集合，如果是声明，则加入局部变量集合
        auto defSym = tac->getDefineSym(); 
        if (defSym)
        {
            symSet.insert(defSym);
            symDefMap[defSym].insert(n);
        }

        // 使用变量列表处理：对列表中每个使用变量，加入函数中出现的变量集合，加入该变量的使用集合
        auto useSymLs = tac->getUseSym();
        for (auto s : useSymLs)
        {
            symSet.insert(s);
            symUseMap[s].insert(n);
        }

        auto& outLs = cfg->get_outNodeList(n);
        for (auto u : outLs)
            if (!vis[u])
                q.push(u);
    }
}

}
}