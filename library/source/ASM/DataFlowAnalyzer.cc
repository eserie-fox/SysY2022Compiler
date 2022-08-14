#include "ASM/DataFlowAnalyzer.hh"
#include "ASM/ControlFlowGraph.hh"
#include <vector>
#include <queue>
#include <cassert>

namespace HaveFunCompiler{
namespace AssemblyBuilder{

// std::vector<size_t> TopSorter::sort(Dir dir) const
// {
//     auto siz = cfg->get_nodes_number();
//     std::vector<size_t> res;
//     res.reserve(siz);
//     std::queue<size_t> q;
//     std::vector<size_t> degree(siz);

//     if (dir == FORWARD)
//     {
//         for (size_t i = cfg->beginIdx(); i < cfg->endIdx(); i = cfg->nextIdx(i))
//         {
//             degree[i] = cfg->get_inDegree(i);
//             if (degree[i] == 0)
//                 q.push(i);
//         }
//     }
//     else
//     {
//         for (size_t i = cfg->beginIdx(); i < cfg->endIdx(); i = cfg->nextIdx(i))
//         {
//             degree[i] = cfg->get_outDegree(i);
//             if (degree[i] == 0)
//                 q.push(i);
//         }
//     }
//     assert(degree[q.front()] == 0);
    
//     while (!q.empty())
//     {
//         auto u = q.front();
//         q.pop();
//         res.push_back(u);
//         auto& ls = dir == FORWARD ? cfg->get_outNodeList(u) : cfg->get_inNodeList(u);
//         for (auto v : ls)
//         {
//             --degree[v];
//             if (degree[v] == 0)
//                 q.push(v);
//         }
//     }

//     #ifdef DEBUG_CHECK
//     cfg->printToDot();
//     size_t cnt = 0;
//     for (size_t i = 0; i < siz; ++i)
//         if (cfg->get_node_dfn(i) != 0)
//             ++cnt;
//     if (res.size() != cnt)
//         throw std::runtime_error("topo_sort error: incomplete");
//     #endif

//     return res;
// }

}
}