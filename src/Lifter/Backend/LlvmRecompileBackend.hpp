#pragma once

#include "Lifter/Backend/IRecompileBackend.hpp"

namespace Lifter::Backend
{
    class LlvmRecompileBackend final : public IRecompileBackend
    {
    public:
        void Recompile(Ir::IIrModule& module, const RecompileRequest& request) const override;
    };
}
