#include "Lifter/Backend/LlvmOptimizer.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif

#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>

#include "Lifter/Ir/IIrModule.hpp"

namespace Lifter::Backend
{
    void LlvmOptimizer::Optimize(Ir::IIrModule& module) const
    {
        llvm::PassBuilder passBuilder;

        llvm::LoopAnalysisManager loopAnalyses;
        llvm::FunctionAnalysisManager functionAnalyses;
        llvm::CGSCCAnalysisManager cgsccAnalyses;
        llvm::ModuleAnalysisManager moduleAnalyses;

        passBuilder.registerModuleAnalyses(moduleAnalyses);
        passBuilder.registerCGSCCAnalyses(cgsccAnalyses);
        passBuilder.registerFunctionAnalyses(functionAnalyses);
        passBuilder.registerLoopAnalyses(loopAnalyses);
        passBuilder.crossRegisterProxies(loopAnalyses, functionAnalyses, cgsccAnalyses, moduleAnalyses);

        llvm::ModulePassManager modulePasses = passBuilder.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
        modulePasses.run(module.GetModule(), moduleAnalyses);
    }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
