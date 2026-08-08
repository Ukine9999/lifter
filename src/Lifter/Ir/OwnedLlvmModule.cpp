#include "Lifter/Ir/OwnedLlvmModule.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

namespace Lifter::Ir
{
    OwnedLlvmModule::OwnedLlvmModule(std::unique_ptr<llvm::LLVMContext> context, std::unique_ptr<llvm::Module> module,
                                     std::string primaryFunction)
        : m_context(std::move(context)), m_module(std::move(module)), m_primaryFunction(std::move(primaryFunction))
    {
    }

    OwnedLlvmModule::~OwnedLlvmModule() = default;

    llvm::Module& OwnedLlvmModule::GetModule()
    {
        return *m_module;
    }

    llvm::LLVMContext& OwnedLlvmModule::GetContext()
    {
        return *m_context;
    }

    std::string OwnedLlvmModule::PrimaryFunctionName() const
    {
        return m_primaryFunction;
    }

    std::string OwnedLlvmModule::Print() const
    {
        std::string text;
        llvm::raw_string_ostream stream(text);
        m_module->print(stream, nullptr);
        return text;
    }
}
