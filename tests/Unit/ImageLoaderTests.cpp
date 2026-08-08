#include <catch2/catch_test_macros.hpp>

#include <cstdint>

#include "Lifter/AddressSpace/FileImageProvider.hpp"
#include "Support/TransformFixture.hpp"

using Lifter::AddressSpace::FileImageProvider;
using Lifter::AddressSpace::MappedAddressSpace;

TEST_CASE("FileImageProvider parses the transform image layout", "[loader]")
{
    const Lifter::Test::TransformFixture fixture(LIFTER_TRANSFORM_PATH);
    const FileImageProvider provider;
    const MappedAddressSpace image = provider.Load(LIFTER_TRANSFORM_PATH);

    CHECK(image.ImageBase() != 0);
    CHECK(image.EntryPoint() >= image.ImageBase());
    CHECK_FALSE(image.Regions().empty());
    CHECK(image.PointerTo(image.ImageBase() + fixture.FunctionRva(), 1) != nullptr);
}

TEST_CASE("FileImageProvider maps TransformCore bytes at its exported virtual address", "[loader]")
{
    const Lifter::Test::TransformFixture fixture(LIFTER_TRANSFORM_PATH);
    const FileImageProvider provider;
    const MappedAddressSpace image = provider.Load(LIFTER_TRANSFORM_PATH);
    const std::uint64_t functionVirtualAddress = image.ImageBase() + fixture.FunctionRva();

    const std::uint8_t* bytes = image.PointerTo(functionVirtualAddress, 16);

    REQUIRE(bytes != nullptr);
    CHECK(image.AvailableBytesFrom(functionVirtualAddress) >= 16);
}

TEST_CASE("FileImageProvider returns null outside mapped ranges", "[loader]")
{
    const FileImageProvider provider;
    const MappedAddressSpace image = provider.Load(LIFTER_TRANSFORM_PATH);

    CHECK(image.PointerTo(UINT64_MAX, 1) == nullptr);
    CHECK(image.AvailableBytesFrom(UINT64_MAX) == 0);
}
