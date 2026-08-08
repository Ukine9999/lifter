#pragma once

#include <memory>

#include "Lifter/Disasm/ControlFlowGraph.hpp"
#include "Lifter/Ir/IIrModule.hpp"

namespace Lifter::Lift
{
    class ILifter
    {
    public:
        virtual ~ILifter() = default;

        virtual std::unique_ptr<Ir::IIrModule> Lift(const Disasm::ControlFlowGraph& graph) const = 0;
    };
}
