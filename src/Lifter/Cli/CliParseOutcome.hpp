#pragma once

#include <string>

#include "Lifter/Cli/CliOptions.hpp"

namespace Lifter::Cli
{
    struct CliParseOutcome
    {
        bool succeeded = false;
        CliOptions options;
        std::string errorMessage;
    };
}
