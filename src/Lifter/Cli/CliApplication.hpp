#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace Lifter::Pipeline
{
    class IBinaryProcessor;
}

namespace Lifter::Config
{
    class IConfigLoader;
}

namespace Lifter::Cli
{
    class CliApplication
    {
    public:
        CliApplication(const Pipeline::IBinaryProcessor& binaryProcessor, const Config::IConfigLoader& configLoader,
                       std::ostream& output, std::ostream& error);

        int Run(int argumentCount, char** argumentValues) const;

        int Run(const std::vector<std::string>& arguments) const;

    private:
        const Pipeline::IBinaryProcessor& m_binaryProcessor;
        const Config::IConfigLoader& m_configLoader;
        std::ostream& m_output;
        std::ostream& m_error;
    };
}
