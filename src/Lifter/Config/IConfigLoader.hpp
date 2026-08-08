#pragma once

#include <string>

#include "Lifter/Config/Configuration.hpp"

namespace Lifter::Config
{
    class IConfigLoader
    {
    public:
        virtual ~IConfigLoader() = default;

        virtual Configuration LoadFile(const std::string& path) const = 0;
        virtual Configuration ParseText(const std::string& text) const = 0;
    };
}
