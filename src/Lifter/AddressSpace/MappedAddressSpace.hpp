#pragma once

#include "Lifter/AddressSpace/IAddressSpace.hpp"

namespace Lifter::AddressSpace
{
    class MappedAddressSpace final : public IAddressSpace
    {
    public:
        MappedAddressSpace() = default;
        MappedAddressSpace(std::uint64_t imageBase, std::uint64_t entryPoint, std::vector<Region> regions);

        std::uint64_t ImageBase() const override;
        std::uint64_t EntryPoint() const override;
        const std::vector<Region>& Regions() const override;
        const std::uint8_t* PointerTo(std::uint64_t virtualAddress, std::uint64_t size) const override;
        std::uint64_t AvailableBytesFrom(std::uint64_t virtualAddress) const override;

    private:
        std::uint64_t m_imageBase = 0;
        std::uint64_t m_entryPoint = 0;
        std::vector<Region> m_regions;
    };
}
