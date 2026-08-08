#pragma once

#include <cstdint>

namespace Lifter::Cli
{
    enum class CliCommand : std::uint32_t
    {
        ShowHelp,
        ShowVersion,
        ProcessBinary,
    };
}
