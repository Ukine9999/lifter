#pragma once

#include <string>

namespace Lifter::Ir
{
    class IIrModule;
}

namespace Lifter::Backend
{
    struct RecompileRequest
    {
        std::string outputPath;
        std::string exportName;
    };

    class IRecompileBackend
    {
    public:
        virtual ~IRecompileBackend() = default;

        virtual void Recompile(Ir::IIrModule& module, const RecompileRequest& request) const = 0;
    };
}
