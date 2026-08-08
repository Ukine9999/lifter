#pragma once

#include "Lifter/Backend/IOptimizer.hpp"

namespace Lifter::Backend
{
    class LlvmOptimizer final : public IOptimizer
    {
    public:
        void Optimize(Ir::IIrModule& module) const override;
    };
}
