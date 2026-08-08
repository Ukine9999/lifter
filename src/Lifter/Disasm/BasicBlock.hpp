#pragma once

#include <cstdint>
#include <vector>

#include "Lifter/Disasm/Instruction.hpp"

namespace Lifter::Disasm
{
    struct BasicBlock
    {
        std::uint64_t startAddress = 0;
        std::vector<DecodedInstruction> instructions;
        std::vector<std::uint64_t> successors;
        bool endsWithReturn = false;
    };
}
