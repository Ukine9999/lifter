#pragma once

#include <string>

#include "Lifter/AddressSpace/MappedAddressSpace.hpp"

namespace Lifter::AddressSpace
{
    class IImageProvider
    {
    public:
        virtual ~IImageProvider() = default;

        virtual MappedAddressSpace Load(const std::string& path) const = 0;
    };
}
