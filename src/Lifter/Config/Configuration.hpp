#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Lifter/Config/InputSource.hpp"
#include "Lifter/Config/OutputSpec.hpp"

namespace Lifter::Config
{
    struct Configuration
    {
        std::uint64_t entry = 0;
        std::string functionName;
        std::vector<InputSource> sources;
        OutputSpec output;
    };
}
