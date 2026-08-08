#include "Lifter/Cli/CliParser.hpp"

#include <charconv>
#include <cstddef>
#include <optional>
#include <string_view>

namespace Lifter::Cli
{
    namespace
    {
        CliParseOutcome Failure(std::string message)
        {
            CliParseOutcome outcome;
            outcome.succeeded = false;
            outcome.errorMessage = std::move(message);
            return outcome;
        }

        bool ParseUnsigned(std::string_view text, std::uint64_t& value)
        {
            int base = 10;
            if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
            {
                base = 16;
                text.remove_prefix(2);
            }

            if (text.empty()) return false;

            std::uint64_t parsed = 0;
            const char* const begin = text.data();
            const char* const end = text.data() + text.size();
            const std::from_chars_result result = std::from_chars(begin, end, parsed, base);

            if (result.ec != std::errc() || result.ptr != end) return false;

            value = parsed;
            return true;
        }
    }

    CliParseOutcome CliParser::Parse(const std::vector<std::string>& arguments) const
    {
        CliOptions options;
        std::optional<CliCommand> discoveryCommand;
        bool sawPipelineToken = false;
        bool sawDirectOption = false;

        for (std::size_t index = 0; index < arguments.size(); ++index)
        {
            const std::string& token = arguments[index];

            if (token == "--help" || token == "-h")
                discoveryCommand = CliCommand::ShowHelp;
            else if (token == "--version")
                discoveryCommand = CliCommand::ShowVersion;
            else if (token == "--compile")
            {
                options.compile = true;
                sawPipelineToken = true;
                sawDirectOption = true;
            }
            else if (token == "--function")
            {
                if (index + 1 >= arguments.size()) return Failure("missing value for --function");

                std::uint64_t functionRva = 0;
                if (!ParseUnsigned(arguments[index + 1], functionRva))
                    return Failure("invalid --function value: " + arguments[index + 1]);

                options.functionRva = functionRva;
                sawPipelineToken = true;
                sawDirectOption = true;
                ++index;
            }
            else if (token == "--name")
            {
                if (index + 1 >= arguments.size()) return Failure("missing value for --name");

                options.functionName = arguments[index + 1];
                sawPipelineToken = true;
                sawDirectOption = true;
                ++index;
            }
            else if (token == "-o" || token == "--output")
            {
                if (index + 1 >= arguments.size()) return Failure("missing value for " + token);

                options.outputPath = arguments[index + 1];
                sawPipelineToken = true;
                sawDirectOption = true;
                ++index;
            }
            else if (token == "--config")
            {
                if (index + 1 >= arguments.size()) return Failure("missing value for --config");

                options.configPath = arguments[index + 1];
                sawPipelineToken = true;
                ++index;
            }
            else if (!token.empty() && token.front() == '-')
                return Failure("unknown option: " + token);
            else
            {
                if (!options.binaryPath.empty()) return Failure("unexpected extra argument: " + token);

                options.binaryPath = token;
            }
        }

        if (discoveryCommand.has_value())
        {
            if (!options.binaryPath.empty() || sawPipelineToken)
                return Failure("discovery commands take no binary or pipeline options");

            options.command = *discoveryCommand;

            CliParseOutcome outcome;
            outcome.succeeded = true;
            outcome.options = std::move(options);
            return outcome;
        }

        if (!options.configPath.empty() && (!options.binaryPath.empty() || sawDirectOption))
            return Failure("--config cannot be combined with a binary or direct processing options");

        if (options.binaryPath.empty() && options.configPath.empty())
            return Failure("no input binary or --config given");

        options.command = CliCommand::ProcessBinary;

        CliParseOutcome outcome;
        outcome.succeeded = true;
        outcome.options = std::move(options);
        return outcome;
    }
}
