#include "Lifter/Composition/CompositionRoot.hpp"

#include <memory>

#include "Lifter/AddressSpace/FileImageProvider.hpp"
#include "Lifter/Backend/LlvmOptimizer.hpp"
#include "Lifter/Backend/LlvmRecompileBackend.hpp"
#include "Lifter/Config/IConfigLoader.hpp"
#include "Lifter/Config/TomlConfigLoader.hpp"
#include "Lifter/Disasm/ZydisDisassembler.hpp"
#include "Lifter/Lift/X86ToLlvmLifter.hpp"
#include "Lifter/Pipeline/BinaryProcessor.hpp"
#include "Lifter/Pipeline/IBinaryProcessor.hpp"

namespace Lifter::Composition
{
    CompositionRoot::CompositionRoot()
    {
        m_binaryProcessor = std::make_shared<Pipeline::BinaryProcessor>(
            std::make_shared<AddressSpace::FileImageProvider>(), std::make_shared<Disasm::ZydisDisassembler>(),
            std::make_shared<Lift::X86ToLlvmLifter>(), std::make_shared<Backend::LlvmOptimizer>(),
            std::make_shared<Backend::LlvmRecompileBackend>());
        m_configLoader = std::make_shared<Config::TomlConfigLoader>();
    }

    const Pipeline::IBinaryProcessor& CompositionRoot::BinaryProcessor() const
    {
        return *m_binaryProcessor;
    }

    const Config::IConfigLoader& CompositionRoot::ConfigLoader() const
    {
        return *m_configLoader;
    }
}
