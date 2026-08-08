#include "Lifter/Pipeline/BinaryProcessor.hpp"

#include <cwchar>
#include <exception>
#include <filesystem>
#include <memory>
#include <utility>

#include "Lifter/Ir/IIrModule.hpp"

namespace Lifter::Pipeline
{
    namespace
    {
        bool PathsReferToSameFile(const std::string& left, const std::string& right)
        {
            std::error_code error;
            if (std::filesystem::equivalent(left, right, error) && !error) return true;

            error.clear();
            const std::filesystem::path absoluteLeft = std::filesystem::absolute(left, error).lexically_normal();
            if (error) return false;

            const std::filesystem::path absoluteRight = std::filesystem::absolute(right, error).lexically_normal();
            if (error) return false;

            return _wcsicmp(absoluteLeft.native().c_str(), absoluteRight.native().c_str()) == 0;
        }
    }

    BinaryProcessor::BinaryProcessor(std::shared_ptr<AddressSpace::IImageProvider> provider,
                                     std::shared_ptr<Disasm::IDisassembler> disassembler,
                                     std::shared_ptr<Lift::ILifter> lifter,
                                     std::shared_ptr<Backend::IOptimizer> optimizer,
                                     std::shared_ptr<Backend::IRecompileBackend> backend)
        : m_provider(std::move(provider)), m_disassembler(std::move(disassembler)), m_lifter(std::move(lifter)),
          m_optimizer(std::move(optimizer)), m_backend(std::move(backend))
    {
    }

    ProcessResult BinaryProcessor::Process(const ProcessRequest& request) const
    {
        ProcessResult result;

        try
        {
            const std::string outputPath =
                request.compile && request.outputPath.empty() ? "recovered.dll" : request.outputPath;

            if (!outputPath.empty() &&
                (PathsReferToSameFile(request.binaryPath, outputPath) ||
                 (request.compile && PathsReferToSameFile(request.binaryPath, outputPath + ".obj"))))
                throw std::runtime_error("process: output and intermediate paths must differ from the input binary");

            const AddressSpace::MappedAddressSpace image = m_provider->Load(request.binaryPath);
            const std::uint64_t address =
                request.functionRva != 0 ? image.ImageBase() + request.functionRva : image.EntryPoint();

            const Disasm::ControlFlowGraph graph =
                m_disassembler->DisassembleFunction(image, address, request.functionName);

            const std::unique_ptr<Ir::IIrModule> module = m_lifter->Lift(graph);
            m_optimizer->Optimize(*module);
            result.irText = module->Print();

            if (request.compile)
            {
                m_backend->Recompile(*module, Backend::RecompileRequest{outputPath, request.functionName});
                result.producedPath = outputPath;
            }

            result.succeeded = true;
        }
        catch (const std::exception& error)
        {
            result.succeeded = false;
            result.diagnostics = error.what();
        }

        return result;
    }
}
