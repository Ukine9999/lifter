#pragma once

#include <cstdint>
#include <string>

#include "Lifter/Cli/CliCommand.hpp"

namespace Lifter::Cli
{
    struct CliOptions
    {
        CliCommand command = CliCommand::ShowHelp;
        std::string binaryPath;
        std::string configPath;
        std::uint64_t functionRva = 0;
        std::string functionName;
        std::string outputPath;
        bool compile = false;
    };
}
