#pragma once

#include "Lifter/Lift/ILifter.hpp"

namespace Lifter::Lift
{
    class X86ToLlvmLifter final : public ILifter
    {
    public:
        std::unique_ptr<Ir::IIrModule> Lift(const Disasm::ControlFlowGraph& graph) const override;
    };
}
