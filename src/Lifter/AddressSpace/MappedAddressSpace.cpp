#include "Lifter/AddressSpace/MappedAddressSpace.hpp"

#include <utility>

namespace Lifter::AddressSpace
{
    MappedAddressSpace::MappedAddressSpace(std::uint64_t imageBase, std::uint64_t entryPoint,
                                           std::vector<Region> regions)
        : m_imageBase(imageBase), m_entryPoint(entryPoint), m_regions(std::move(regions))
    {
    }

    std::uint64_t MappedAddressSpace::ImageBase() const
    {
        return m_imageBase;
    }

    std::uint64_t MappedAddressSpace::EntryPoint() const
    {
        return m_entryPoint;
    }

    const std::vector<Region>& MappedAddressSpace::Regions() const
    {
        return m_regions;
    }

    const std::uint8_t* MappedAddressSpace::PointerTo(std::uint64_t virtualAddress, std::uint64_t size) const
    {
        for (const Region& region : m_regions)
        {
            const std::uint64_t start = region.virtualAddress;
            if (virtualAddress < start) continue;

            const std::uint64_t offset = virtualAddress - start;
            const std::uint64_t regionSize = region.bytes.size();
            if (offset <= regionSize && size <= regionSize - offset) return region.bytes.data() + offset;
        }

        return nullptr;
    }

    std::uint64_t MappedAddressSpace::AvailableBytesFrom(std::uint64_t virtualAddress) const
    {
        for (const Region& region : m_regions)
        {
            const std::uint64_t start = region.virtualAddress;
            if (virtualAddress < start) continue;

            const std::uint64_t offset = virtualAddress - start;
            const std::uint64_t regionSize = region.bytes.size();
            if (offset < regionSize) return regionSize - offset;
        }

        return 0;
    }
}
