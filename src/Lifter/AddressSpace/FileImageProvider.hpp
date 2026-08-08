#pragma once

#include "Lifter/AddressSpace/IImageProvider.hpp"

namespace Lifter::AddressSpace
{
    class FileImageProvider final : public IImageProvider
    {
    public:
        MappedAddressSpace Load(const std::string& path) const override;
    };
}
