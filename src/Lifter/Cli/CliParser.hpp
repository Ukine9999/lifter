#pragma once

#include <string>
#include <vector>

#include "Lifter/Cli/CliParseOutcome.hpp"

namespace Lifter::Cli
{
    class CliParser
    {
    public:
        CliParseOutcome Parse(const std::vector<std::string>& arguments) const;
    };
}
