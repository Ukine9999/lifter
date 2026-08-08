#pragma once

#include <string>

namespace llvm
{
    class Module;
    class LLVMContext;
}

namespace Lifter::Ir
{
    class IIrModule
    {
    public:
        virtual ~IIrModule() = default;

        virtual llvm::Module& GetModule() = 0;

        virtual llvm::LLVMContext& GetContext() = 0;

        virtual std::string PrimaryFunctionName() const = 0;

        virtual std::string Print() const = 0;
    };
}
