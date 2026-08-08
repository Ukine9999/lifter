#pragma once

#include <cstdint>
#include <string>

#include "Lifter/AddressSpace/IAddressSpace.hpp"
#include "Lifter/Disasm/ControlFlowGraph.hpp"

namespace Lifter::Disasm
{
    class IDisassembler
    {
    public:
        virtual ~IDisassembler() = default;

        virtual ControlFlowGraph DisassembleFunction(const AddressSpace::IAddressSpace& image,
                                                     std::uint64_t entryAddress, const std::string& name) const = 0;
    };
}
