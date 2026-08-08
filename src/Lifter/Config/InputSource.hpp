#pragma once

#include <string>

#include "Lifter/Config/ESourceKind.hpp"

namespace Lifter::Config
{
    struct InputSource
    {
        ESourceKind kind = ESourceKind::FILE;
        std::string path;
    };
}
