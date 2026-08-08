#include "Lifter/Disasm/ControlFlowGraph.hpp"

namespace Lifter::Disasm
{
    const BasicBlock* ControlFlowGraph::BlockAt(std::uint64_t address) const
    {
        for (const BasicBlock& block : blocks)
        {
            if (block.startAddress == address) return &block;
        }

        return nullptr;
    }
}
