#pragma once

#include <memory>
#include <string>

#include "Lifter/Ir/IIrModule.hpp"

namespace llvm
{
    class Module;
    class LLVMContext;
}

namespace Lifter::Ir
{
    class OwnedLlvmModule final : public IIrModule
    {
    public:
        OwnedLlvmModule(std::unique_ptr<llvm::LLVMContext> context, std::unique_ptr<llvm::Module> module,
                        std::string primaryFunction);

        ~OwnedLlvmModule() override;

        llvm::Module& GetModule() override;

        llvm::LLVMContext& GetContext() override;

        std::string PrimaryFunctionName() const override;

        std::string Print() const override;

    private:
        std::unique_ptr<llvm::LLVMContext> m_context;
        std::unique_ptr<llvm::Module> m_module;
        std::string m_primaryFunction;
    };
}
