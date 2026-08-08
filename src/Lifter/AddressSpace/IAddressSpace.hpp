#pragma once

#include <cstdint>
#include <vector>

namespace Lifter::AddressSpace
{
    struct Region
    {
        std::uint64_t virtualAddress;
        std::vector<std::uint8_t> bytes;
    };

    class IAddressSpace
    {
    public:
        virtual ~IAddressSpace() = default;

        virtual std::uint64_t ImageBase() const = 0;
        virtual std::uint64_t EntryPoint() const = 0;
        virtual const std::vector<Region>& Regions() const = 0;
        virtual const std::uint8_t* PointerTo(std::uint64_t virtualAddress, std::uint64_t size) const = 0;
        virtual std::uint64_t AvailableBytesFrom(std::uint64_t virtualAddress) const = 0;
    };
}
