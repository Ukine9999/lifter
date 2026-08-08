#pragma once

#include <memory>

#include "Lifter/AddressSpace/IImageProvider.hpp"
#include "Lifter/Backend/IOptimizer.hpp"
#include "Lifter/Backend/IRecompileBackend.hpp"
#include "Lifter/Disasm/IDisassembler.hpp"
#include "Lifter/Lift/ILifter.hpp"
#include "Lifter/Pipeline/IBinaryProcessor.hpp"

namespace Lifter::Pipeline
{
    class BinaryProcessor final : public IBinaryProcessor
    {
    public:
        BinaryProcessor(std::shared_ptr<AddressSpace::IImageProvider> provider,
                        std::shared_ptr<Disasm::IDisassembler> disassembler, std::shared_ptr<Lift::ILifter> lifter,
                        std::shared_ptr<Backend::IOptimizer> optimizer,
                        std::shared_ptr<Backend::IRecompileBackend> backend);

        ProcessResult Process(const ProcessRequest& request) const override;

    private:
        std::shared_ptr<AddressSpace::IImageProvider> m_provider;
        std::shared_ptr<Disasm::IDisassembler> m_disassembler;
        std::shared_ptr<Lift::ILifter> m_lifter;
        std::shared_ptr<Backend::IOptimizer> m_optimizer;
        std::shared_ptr<Backend::IRecompileBackend> m_backend;
    };
}
