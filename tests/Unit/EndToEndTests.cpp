#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <string>

#include "Lifter/AddressSpace/FileImageProvider.hpp"
#include "Lifter/Backend/LlvmOptimizer.hpp"
#include "Lifter/Backend/LlvmRecompileBackend.hpp"
#include "Lifter/Disasm/ZydisDisassembler.hpp"
#include "Lifter/Lift/X86ToLlvmLifter.hpp"
#include "Lifter/Pipeline/BinaryProcessor.hpp"
#include "Support/TransformFixture.hpp"

using namespace Lifter;

namespace
{
    Pipeline::BinaryProcessor MakeProcessor()
    {
        return Pipeline::BinaryProcessor(
            std::make_shared<AddressSpace::FileImageProvider>(), std::make_shared<Disasm::ZydisDisassembler>(),
            std::make_shared<Lift::X86ToLlvmLifter>(), std::make_shared<Backend::LlvmOptimizer>(),
            std::make_shared<Backend::LlvmRecompileBackend>());
    }
}

TEST_CASE("lifter compile output is I/O-equivalent to TransformCore", "[e2e]")
{
    const std::string outputPath = "lifter_e2e_recovered.dll";
    const Test::TransformFixture original(LIFTER_TRANSFORM_PATH);
    const Pipeline::BinaryProcessor processor = MakeProcessor();

    Pipeline::ProcessRequest request;
    request.binaryPath = LIFTER_TRANSFORM_PATH;
    request.compile = true;
    request.outputPath = outputPath;
    request.functionRva = original.FunctionRva();
    request.functionName = "TransformCore";

    const Pipeline::ProcessResult result = processor.Process(request);
    REQUIRE(result.succeeded);
    REQUIRE(result.producedPath == outputPath);

    const Test::TransformFixture recovered(outputPath);

    CHECK(recovered.Evaluate("hello", 5) == original.Evaluate("hello", 5));
    CHECK(recovered.Evaluate("a", 1) == original.Evaluate("a", 1));
    CHECK(recovered.Evaluate("", 0) == original.Evaluate("", 0));

    std::uint64_t randomState = 0x0f1e2d3c4b5a6978ull;
    const auto nextRandom = [&randomState]()
    {
        randomState ^= randomState << 13;
        randomState ^= randomState >> 7;
        randomState ^= randomState << 17;
        return randomState;
    };

    int mismatches = 0;
    for (int iteration = 0; iteration < 200000; ++iteration)
    {
        const unsigned length = static_cast<unsigned>(nextRandom() % 64);
        std::uint8_t buffer[64];
        for (unsigned index = 0; index < length; ++index)
            buffer[index] = static_cast<std::uint8_t>(nextRandom());

        if (original.Evaluate(buffer, length) != recovered.Evaluate(buffer, length)) ++mismatches;
    }

    CHECK(mismatches == 0);
}
