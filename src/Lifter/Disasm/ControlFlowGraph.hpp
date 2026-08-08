#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Lifter/Disasm/BasicBlock.hpp"

namespace Lifter::Disasm
{
    struct ControlFlowGraph
    {
        std::string name;
        std::uint64_t entryAddress = 0;
        std::vector<BasicBlock> blocks;

        const BasicBlock* BlockAt(std::uint64_t address) const;
    };
}
