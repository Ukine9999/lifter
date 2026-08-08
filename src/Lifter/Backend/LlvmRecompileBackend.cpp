#include "Lifter/Backend/LlvmRecompileBackend.hpp"

#include <stdexcept>
#include <string>
#include <vector>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include <lld/Common/Driver.h>

#include "Lifter/Ir/IIrModule.hpp"

LLD_HAS_DRIVER(coff)

namespace Lifter::Backend
{
    namespace
    {
        const char* TargetTriple()
        {
            return "x86_64-pc-windows-msvc";
        }

        void EmitObject(llvm::Module& module, const std::string& objectPath)
        {
            LLVMInitializeX86TargetInfo();
            LLVMInitializeX86Target();
            LLVMInitializeX86TargetMC();
            LLVMInitializeX86AsmParser();
            LLVMInitializeX86AsmPrinter();

            std::string lookupError;
            const llvm::Target* target = llvm::TargetRegistry::lookupTarget(TargetTriple(), lookupError);
            if (!target) throw std::runtime_error("recompile: target lookup failed: " + lookupError);

            llvm::TargetOptions targetOptions;
            llvm::TargetMachine* targetMachine =
                target->createTargetMachine(TargetTriple(), "generic", "", targetOptions, llvm::Reloc::PIC_,
                                            std::nullopt, llvm::CodeGenOptLevel::Default);

            module.setDataLayout(targetMachine->createDataLayout());
            module.setTargetTriple(TargetTriple());

            std::error_code errorCode;
            llvm::raw_fd_ostream objectStream(objectPath, errorCode, llvm::sys::fs::OF_None);
            if (errorCode) throw std::runtime_error("recompile: cannot open object file: " + errorCode.message());

            llvm::legacy::PassManager passManager;
            if (targetMachine->addPassesToEmitFile(passManager, objectStream, nullptr,
                                                   llvm::CodeGenFileType::ObjectFile))
                throw std::runtime_error("recompile: target cannot emit an object file");

            passManager.run(module);
            objectStream.flush();
        }

        void LinkSharedLibrary(const std::string& objectPath, const std::string& outputPath,
                               const std::string& exportName)
        {
            std::vector<std::string> arguments = {
                "lld-link",           "/dll",    "/noentry", "/nodefaultlib", "/export:" + exportName,
                "/out:" + outputPath, objectPath};

            std::vector<const char*> rawArguments;
            rawArguments.reserve(arguments.size());
            for (const std::string& argument : arguments)
                rawArguments.push_back(argument.c_str());

            if (!lld::coff::link(rawArguments, llvm::outs(), llvm::errs(), false, false))
                throw std::runtime_error("recompile: lld link failed");
        }
    }

    void LlvmRecompileBackend::Recompile(Ir::IIrModule& module, const RecompileRequest& request) const
    {
        const std::string objectPath = request.outputPath + ".obj";
        EmitObject(module.GetModule(), objectPath);
        LinkSharedLibrary(objectPath, request.outputPath, request.exportName);
    }
}
