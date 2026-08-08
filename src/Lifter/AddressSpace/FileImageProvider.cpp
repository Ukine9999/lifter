#include "Lifter/AddressSpace/FileImageProvider.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>

#include <LIEF/PE.hpp>

namespace Lifter::AddressSpace
{
    MappedAddressSpace FileImageProvider::Load(const std::string& path) const
    {
        const std::unique_ptr<LIEF::PE::Binary> binary = LIEF::PE::Parser::parse(path);
        if (!binary) throw std::runtime_error("pe: LIEF failed to parse " + path);

        const std::uint64_t imageBase = binary->optional_header().imagebase();
        const std::uint64_t entryPoint = binary->entrypoint();

        std::vector<Region> regions;
        for (const LIEF::PE::Section& section : binary->sections())
        {
            const auto content = section.content();
            const std::uint64_t mappedSize = std::max<std::uint64_t>(section.virtual_size(), content.size());

            Region region;
            region.virtualAddress = imageBase + section.virtual_address();
            region.bytes.assign(static_cast<std::size_t>(mappedSize), 0);

            if (!content.empty()) std::memcpy(region.bytes.data(), content.data(), content.size());

            regions.push_back(std::move(region));
        }

        return MappedAddressSpace(imageBase, entryPoint, std::move(regions));
    }
}
