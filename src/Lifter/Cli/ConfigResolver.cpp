#include "Lifter/Cli/ConfigResolver.hpp"

#include <utility>

#include "Lifter/Config/ConfigError.hpp"

namespace Lifter::Cli
{
    Config::Configuration ConfigurationFromOptions(const CliOptions& options)
    {
        Config::Configuration configuration;
        configuration.entry = options.functionRva;
        configuration.functionName = options.functionName;

        Config::InputSource source;
        source.kind = Config::ESourceKind::FILE;
        source.path = options.binaryPath;
        configuration.sources.push_back(std::move(source));

        configuration.output.mode = options.compile ? Config::EOutputMode::COMPILE : Config::EOutputMode::IR;
        configuration.output.path = options.outputPath;
        return configuration;
    }

    Pipeline::ProcessRequest RequestFromConfiguration(const Config::Configuration& configuration)
    {
        if (configuration.functionName.empty())
            throw Config::ConfigError(
                "config: a target function name is required (set 'name' in the config or --name)");

        const Config::InputSource* fileSource = nullptr;
        for (const Config::InputSource& source : configuration.sources)
        {
            if (fileSource != nullptr) throw Config::ConfigError("config: exactly one file input source is supported");

            fileSource = &source;
        }

        if (fileSource == nullptr || fileSource->path.empty())
            throw Config::ConfigError("config: a file input source with a path is required");

        Pipeline::ProcessRequest request;
        request.binaryPath = fileSource->path;
        request.functionRva = configuration.entry;
        request.functionName = configuration.functionName;
        request.compile = configuration.output.mode == Config::EOutputMode::COMPILE;
        request.outputPath = configuration.output.path;
        return request;
    }
}
