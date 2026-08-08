#pragma once

namespace Lifter::Ir
{
    class IIrModule;
}

namespace Lifter::Backend
{
    class IOptimizer
    {
    public:
        virtual ~IOptimizer() = default;

        virtual void Optimize(Ir::IIrModule& module) const = 0;
    };
}
