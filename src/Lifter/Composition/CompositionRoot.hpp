#pragma once

#include <memory>

namespace Lifter::Pipeline
{
    class IBinaryProcessor;
}

namespace Lifter::Config
{
    class IConfigLoader;
}

namespace Lifter::Composition
{
    class CompositionRoot
    {
    public:
        CompositionRoot();

        const Pipeline::IBinaryProcessor& BinaryProcessor() const;

        const Config::IConfigLoader& ConfigLoader() const;

    private:
        std::shared_ptr<Pipeline::IBinaryProcessor> m_binaryProcessor;
        std::shared_ptr<Config::IConfigLoader> m_configLoader;
    };
}
