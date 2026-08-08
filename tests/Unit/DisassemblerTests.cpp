#include <catch2/catch_test_macros.hpp>

#include <cstdint>

#include "Lifter/AddressSpace/FileImageProvider.hpp"
#include "Lifter/Disasm/ZydisDisassembler.hpp"
#include "Support/TransformFixture.hpp"

using namespace Lifter;

namespace
{
    Disasm::ControlFlowGraph DisassembleTransformCore()
    {
        const Test::TransformFixture fixture(LIFTER_TRANSFORM_PATH);
        const AddressSpace::FileImageProvider loader;
        const AddressSpace::MappedAddressSpace image = loader.Load(LIFTER_TRANSFORM_PATH);
        const Disasm::ZydisDisassembler disassembler;
        return disassembler.DisassembleFunction(image, image.ImageBase() + fixture.FunctionRva(), "TransformCore");
    }
}

TEST_CASE("ZydisDisassembler recovers the TransformCore control-flow graph", "[disasm]")
{
    const Disasm::ControlFlowGraph graph = DisassembleTransformCore();

    CHECK(graph.blocks.size() >= 5);

    const Disasm::BasicBlock* entry = graph.BlockAt(graph.entryAddress);
    REQUIRE(entry != nullptr);
    REQUIRE_FALSE(entry->instructions.empty());
    CHECK(entry->instructions.front().address == graph.entryAddress);
}

TEST_CASE("TransformCore recovery preserves the loop as a back-edge", "[disasm]")
{
    const Disasm::ControlFlowGraph graph = DisassembleTransformCore();

    bool hasBackEdge = false;
    for (const Disasm::BasicBlock& block : graph.blocks)
    {
        for (const std::uint64_t successor : block.successors)
        {
            if (successor <= block.startAddress) hasBackEdge = true;
        }
    }

    CHECK(hasBackEdge);
}

TEST_CASE("TransformCore recovery decodes the FNV prime multiply", "[disasm]")
{
    const Disasm::ControlFlowGraph graph = DisassembleTransformCore();

    bool foundFnvPrime = false;
    for (const Disasm::BasicBlock& block : graph.blocks)
    {
        for (const Disasm::DecodedInstruction& instruction : block.instructions)
        {
            for (const Disasm::Operand& operand : instruction.operands)
            {
                if (operand.kind == Disasm::OperandKind::Immediate && operand.immediate.value == 0x100000001b3ull)
                    foundFnvPrime = true;
            }
        }
    }

    CHECK(foundFnvPrime);
}

TEST_CASE("TransformCore recovery marks a returning block", "[disasm]")
{
    const Disasm::ControlFlowGraph graph = DisassembleTransformCore();

    bool hasReturn = false;
    for (const Disasm::BasicBlock& block : graph.blocks)
    {
        if (block.endsWithReturn) hasReturn = true;
    }

    CHECK(hasReturn);
}
