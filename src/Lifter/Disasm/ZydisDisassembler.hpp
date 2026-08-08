#pragma once

#include "Lifter/Disasm/IDisassembler.hpp"

namespace Lifter::Disasm
{
    class ZydisDisassembler final : public IDisassembler
    {
    public:
        ControlFlowGraph DisassembleFunction(const AddressSpace::IAddressSpace& image, std::uint64_t entryAddress,
                                             const std::string& name) const override;
    };
}
