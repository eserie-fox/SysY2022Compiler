#include <gtest/gtest.h>
#include "ASM/ControlFlowGraph.hh"
#include "ASM/LiveAnalyzer.hh"
#include "ASM/arm/RegAllocator.hh"
#include "TAC/TAC.hh"
#include <vector>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include "Driver.hh"
#include "TACDriver.hh"

using namespace HaveFunCompiler;
using namespace HaveFunCompiler::ThreeAddressCode;
using namespace HaveFunCompiler::AssemblyBuilder;

std::ostream& operator<<(std::ostream& os, const SymLiveInfo& liveInfo);

std::ostream& operator<<(std::ostream& os, const SymAttribute& attr)
{
    switch (attr.attr.store_type)
    {
    case SymAttribute::INT_REG:
        os << "r";
        break;
    case SymAttribute::FLOAT_REG:
        os << "s";
        break;
    case SymAttribute::STACK_VAR:
        os << "varStack ⬆+";
        break;
    case SymAttribute::STACK_PARAM:
        os << "paramStack ⬇+";
        break;
    default:
        break;
    }
    os << attr.value << '\n';
    return os;
}

bool operator==(const SymAttribute& o1, const SymAttribute& o2)
{
    return o1.attr.store_type == o2.attr.store_type && o1.value == o2.value;
}

struct regAllocStatic
{
    SymLiveInfo occupy;
    std::vector<std::shared_ptr<Symbol>> syms;

    void addSym(std::shared_ptr<Symbol> sym, const std::set<LiveInterval> &occupied)
    {
        for (auto e : occupied)
            occupy.addUncoveredLiveInterval(e);
        syms.push_back(sym);
    }
};

std::ostream& operator<<(std::ostream &os, const regAllocStatic &o)
{
    os << "occupied ranges: ";
    for (auto &e : o.occupy.liveIntervalSet)
        os << '[' << e.first << ", " << e.second << "] ";
    os << "\nalloc to syms: ";
    for (auto sym : o.syms)
        os << sym->get_tac_name(true) << " ";
    os << '\n';
    return os;
}

TEST(RegAllocTest, test)
{
    HaveFunCompiler::Parser::Driver driver;
    HaveFunCompiler::Parser::TACDriver tacdriver;
    TACListPtr tac_list;

    ASSERT_EQ(driver.parse("test.txt"), true);

    std::stringstream ss;
    driver.print(ss) << "\n";
    tacdriver.parse(ss);
    tac_list = tacdriver.get_tacbuilder()->GetTACList();

    std::shared_ptr<ControlFlowGraph> cfg = std::make_shared<ControlFlowGraph>(tac_list);
    cfg->printToDot();

    LiveAnalyzer liveAnalyzer(cfg);
    auto symSet = liveAnalyzer.get_allSymSet();
    RegAllocator regAllocator(liveAnalyzer);

    // 统计信息，判断是否出现冲突的分配
    regAllocStatic r[RegAllocator::intRegPoolSize], s[RegAllocator::floatRegPoolSize];


    for (auto sym : symSet)
    {
        std::cout << "sym \"" << sym->get_tac_name(true) << "\":\n";
        std::cout << "LiveInfo:\n";
        auto liveInfo = liveAnalyzer.get_symLiveInfo(sym);
        if (liveInfo)
            std::cout << *liveInfo;

        std::cout << "AddrInfo: ";
        auto symAttr = regAllocator.get_SymAttribute(sym);   // 先测虚拟寄存器变量
        std::cout << symAttr << '\n';

        // 统计信息
        if (symAttr.attr.store_type == SymAttribute::INT_REG)
            r[symAttr.value].addSym(sym, liveInfo->liveIntervalSet);
        else if (symAttr.attr.store_type == SymAttribute::FLOAT_REG)
            s[symAttr.value].addSym(sym, liveInfo->liveIntervalSet);
    }

    // 输出统计信息
    for (int i = 0; i < RegAllocator::intRegPoolSize; ++i)
    {
        if (r[i].syms.size() == 0)  continue;
        std::cout << "r" << i << '\n';
        std::cout << r[i];
        std::cout << '\n';
    }
    for (int i = 0; i < RegAllocator::floatRegPoolSize; ++i)
    {
        if (s[i].syms.size() == 0)  continue;
        std::cout << "s" << i << '\n';
        std::cout << s[i];
        std::cout << '\n';
    }
}