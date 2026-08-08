#pragma once

#include <string>

#include "Lifter/Config/EOutputMode.hpp"

namespace Lifter::Config
{
    struct OutputSpec
    {
        EOutputMode mode = EOutputMode::IR;
        std::string path;
    };
}
