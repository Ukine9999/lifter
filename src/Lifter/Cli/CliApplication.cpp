#include "Lifter/Cli/CliApplication.hpp"

#include <fstream>
#include <ostream>

#include "Lifter/Cli/CliParser.hpp"
#include "Lifter/Cli/ConfigResolver.hpp"
#include "Lifter/Config/ConfigError.hpp"
#include "Lifter/Config/IConfigLoader.hpp"
#include "Lifter/Pipeline/IBinaryProcessor.hpp"

#ifndef LIFTER_VERSION
#define LIFTER_VERSION "0.0.0-dev"
#endif

namespace Lifter::Cli
{
    namespace
    {
        constexpr int ExitSuccess = 0;
        constexpr int ExitUsageError = 2;
        constexpr int ExitProcessingFailed = 4;

        void WriteUsage(std::ostream& stream)
        {
            stream << "lifter " LIFTER_VERSION " - experimental x86-64 PE lifting pipeline\n\n"
                   << "usage:\n"
                   << "  lifter <binary> --name <symbol> [--function <rva>] [-o out.ll]\n"
                   << "  lifter <binary> --name <symbol> --compile [--function <rva>] [-o out.dll]\n"
                   << "  lifter --config <file.toml>\n"
                   << "  lifter --help\n"
                   << "  lifter --version\n";
        }
    }

    CliApplication::CliApplication(const Pipeline::IBinaryProcessor& binaryProcessor,
                                   const Config::IConfigLoader& configLoader, std::ostream& output, std::ostream& error)
        : m_binaryProcessor(binaryProcessor), m_configLoader(configLoader), m_output(output), m_error(error)
    {
    }

    int CliApplication::Run(int argumentCount, char** argumentValues) const
    {
        std::vector<std::string> arguments;
        for (int index = 1; index < argumentCount; ++index)
            arguments.emplace_back(argumentValues[index]);

        return Run(arguments);
    }

    int CliApplication::Run(const std::vector<std::string>& arguments) const
    {
        const CliParser parser;
        const CliParseOutcome outcome = parser.Parse(arguments);

        if (!outcome.succeeded)
        {
            m_error << "error: " << outcome.errorMessage << "\n\n";
            WriteUsage(m_error);
            return ExitUsageError;
        }

        switch (outcome.options.command)
        {
            case CliCommand::ShowHelp:
                WriteUsage(m_output);
                return ExitSuccess;

            case CliCommand::ShowVersion:
                m_output << "lifter " LIFTER_VERSION "\n";
                return ExitSuccess;

            case CliCommand::ProcessBinary:
            {
                Pipeline::ProcessRequest request;
                try
                {
                    const Config::Configuration configuration =
                        outcome.options.configPath.empty() ? ConfigurationFromOptions(outcome.options)
                                                           : m_configLoader.LoadFile(outcome.options.configPath);
                    request = RequestFromConfiguration(configuration);
                }
                catch (const Config::ConfigError& error)
                {
                    m_error << "error: " << error.what() << "\n";
                    return ExitUsageError;
                }

                const Pipeline::ProcessResult processResult = m_binaryProcessor.Process(request);

                if (!processResult.succeeded)
                {
                    m_error << "error: " << processResult.diagnostics << "\n";
                    return ExitProcessingFailed;
                }

                if (request.compile)
                {
                    m_output << "wrote " << processResult.producedPath << "\n";
                    return ExitSuccess;
                }

                if (!request.outputPath.empty())
                {
                    std::ofstream stream(request.outputPath, std::ios::binary);
                    if (!stream)
                    {
                        m_error << "error: failed to open output: " << request.outputPath << "\n";
                        return ExitProcessingFailed;
                    }

                    stream << processResult.irText;
                    if (!stream)
                    {
                        m_error << "error: failed to write output: " << request.outputPath << "\n";
                        return ExitProcessingFailed;
                    }

                    m_output << "wrote " << request.outputPath << "\n";
                    return ExitSuccess;
                }

                m_output << processResult.irText;
                return ExitSuccess;
            }
        }

        return ExitUsageError;
    }
}
