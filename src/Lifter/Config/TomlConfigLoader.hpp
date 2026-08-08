#pragma once

#include "Lifter/Config/IConfigLoader.hpp"

namespace Lifter::Config
{
    class TomlConfigLoader final : public IConfigLoader
    {
    public:
        Configuration LoadFile(const std::string& path) const override;
        Configuration ParseText(const std::string& text) const override;
    };
}
