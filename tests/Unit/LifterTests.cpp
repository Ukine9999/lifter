#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

#include "Lifter/AddressSpace/FileImageProvider.hpp"
#include "Lifter/Backend/LlvmOptimizer.hpp"
#include "Lifter/Disasm/ZydisDisassembler.hpp"
#include "Lifter/Ir/IIrModule.hpp"
#include "Lifter/Lift/X86ToLlvmLifter.hpp"
#include "Support/TransformFixture.hpp"

using namespace Lifter;

namespace
{
    std::unique_ptr<Ir::IIrModule> LiftTransformCore()
    {
        const Test::TransformFixture fixture(LIFTER_TRANSFORM_PATH);
        const AddressSpace::FileImageProvider loader;
        const AddressSpace::MappedAddressSpace image = loader.Load(LIFTER_TRANSFORM_PATH);
        const Disasm::ZydisDisassembler disassembler;
        const Disasm::ControlFlowGraph graph =
            disassembler.DisassembleFunction(image, image.ImageBase() + fixture.FunctionRva(), "TransformCore");
        const Lift::X86ToLlvmLifter lifter;
        return lifter.Lift(graph);
    }

    std::size_t CountOccurrences(const std::string& haystack, const std::string& needle)
    {
        std::size_t count = 0;
        for (std::size_t position = haystack.find(needle); position != std::string::npos;
             position = haystack.find(needle, position + needle.size()))
            ++count;
        return count;
    }

    Disasm::Operand RegisterOperand(Disasm::GpRegister reg)
    {
        Disasm::Operand operand;
        operand.kind = Disasm::OperandKind::Register;
        operand.reg.reg = reg;
        operand.reg.widthBits = 64;
        return operand;
    }

    Disasm::Operand ImmediateOperand(std::uint64_t value)
    {
        Disasm::Operand operand;
        operand.kind = Disasm::OperandKind::Immediate;
        operand.immediate.value = value;
        operand.immediate.widthBits = 64;
        return operand;
    }

    Disasm::DecodedInstruction ReturnInstruction(std::uint64_t address)
    {
        Disasm::DecodedInstruction instruction;
        instruction.address = address;
        instruction.mnemonic = Disasm::Mnemonic::Ret;
        return instruction;
    }

    std::string LiftFailure(const Disasm::ControlFlowGraph& graph)
    {
        const Lift::X86ToLlvmLifter lifter;
        try
        {
            lifter.Lift(graph);
        }
        catch (const std::runtime_error& error)
        {
            return error.what();
        }

        return {};
    }
}

TEST_CASE("lifting TransformCore yields a whole-function i64 definition", "[lift]")
{
    const std::unique_ptr<Ir::IIrModule> module = LiftTransformCore();

    CHECK(module->PrimaryFunctionName() == "TransformCore");

    const std::string ir = module->Print();
    CHECK(ir.find("define i64 @TransformCore(i64 %0, i64 %1, i64 %2, i64 %3)") != std::string::npos);
    CHECK(ir.find("ret i64") != std::string::npos);
}

TEST_CASE("the lifted CFG is structural with multiple blocks and branches", "[lift]")
{
    const std::unique_ptr<Ir::IIrModule> module = LiftTransformCore();
    const std::string ir = module->Print();

    CHECK(CountOccurrences(ir, "\nb") >= 5);
    CHECK(ir.find("br ") != std::string::npos);
}

TEST_CASE("optimization preserves the loop as a phi rather than unrolling", "[lift]")
{
    const std::unique_ptr<Ir::IIrModule> module = LiftTransformCore();
    const Backend::LlvmOptimizer optimizer;
    optimizer.Optimize(*module);

    const std::string ir = module->Print();

    CHECK(ir.find("phi i64") != std::string::npos);
    CHECK(ir.find("1099511628211") != std::string::npos);
    CHECK(ir.find("ret i64") != std::string::npos);
}

TEST_CASE("lifting rejects instruction-pointer-relative memory", "[lift]")
{
    Disasm::Operand memory;
    memory.kind = Disasm::OperandKind::Memory;
    memory.memory.widthBits = 64;
    memory.memory.isInstructionPointerRelative = true;

    Disasm::DecodedInstruction move;
    move.address = 0x1000;
    move.mnemonic = Disasm::Mnemonic::Mov;
    move.operands = {RegisterOperand(Disasm::GpRegister::Rax), memory};

    Disasm::BasicBlock block;
    block.startAddress = 0x1000;
    block.instructions = {move, ReturnInstruction(0x1001)};
    block.endsWithReturn = true;

    Disasm::ControlFlowGraph graph;
    graph.name = "RipRelative";
    graph.entryAddress = block.startAddress;
    graph.blocks = {block};

    CHECK(LiftFailure(graph) == "lift: instruction-pointer-relative memory is not supported");
}

TEST_CASE("lifting guards synthetic stack accesses at runtime", "[lift]")
{
    Disasm::Operand memory;
    memory.kind = Disasm::OperandKind::Memory;
    memory.memory.base = Disasm::GpRegister::Rsp;
    memory.memory.displacement = 600;
    memory.memory.widthBits = 64;

    Disasm::DecodedInstruction move;
    move.address = 0x1000;
    move.mnemonic = Disasm::Mnemonic::Mov;
    move.operands = {RegisterOperand(Disasm::GpRegister::Rax), memory};

    Disasm::BasicBlock block;
    block.startAddress = 0x1000;
    block.instructions = {move, ReturnInstruction(0x1001)};
    block.endsWithReturn = true;

    Disasm::ControlFlowGraph graph;
    graph.name = "GuardedStackAccess";
    graph.entryAddress = block.startAddress;
    graph.blocks = {block};

    const Lift::X86ToLlvmLifter lifter;
    const std::unique_ptr<Ir::IIrModule> module = lifter.Lift(graph);
    const std::string ir = module->Print();
    CHECK(ir.find("llvm.trap") != std::string::npos);
    CHECK(ir.find("unreachable") != std::string::npos);
}

TEST_CASE("lifting rejects synthetic stack addresses copied into general registers", "[lift]")
{
    Disasm::DecodedInstruction move;
    move.address = 0x1000;
    move.mnemonic = Disasm::Mnemonic::Mov;
    move.operands = {RegisterOperand(Disasm::GpRegister::Rax), RegisterOperand(Disasm::GpRegister::Rsp)};

    Disasm::BasicBlock block;
    block.startAddress = 0x1000;
    block.instructions = {move, ReturnInstruction(0x1001)};
    block.endsWithReturn = true;

    Disasm::ControlFlowGraph graph;
    graph.name = "DerivedStackAddress";
    graph.entryAddress = block.startAddress;
    graph.blocks = {block};

    CHECK(LiftFailure(graph) == "lift: derived synthetic-stack addresses are not supported");
}

TEST_CASE("lifting rejects flags carried across basic blocks", "[lift]")
{
    Disasm::DecodedInstruction compare;
    compare.address = 0x1000;
    compare.mnemonic = Disasm::Mnemonic::Cmp;
    compare.operands = {RegisterOperand(Disasm::GpRegister::Rax), ImmediateOperand(0)};

    Disasm::DecodedInstruction jump;
    jump.address = 0x1001;
    jump.mnemonic = Disasm::Mnemonic::Jmp;

    Disasm::BasicBlock comparisonBlock;
    comparisonBlock.startAddress = 0x1000;
    comparisonBlock.instructions = {compare, jump};
    comparisonBlock.successors = {0x2000};

    Disasm::DecodedInstruction conditionalJump;
    conditionalJump.address = 0x2000;
    conditionalJump.mnemonic = Disasm::Mnemonic::ConditionalJump;
    conditionalJump.condition = Disasm::Condition::Equal;

    Disasm::BasicBlock conditionBlock;
    conditionBlock.startAddress = 0x2000;
    conditionBlock.instructions = {conditionalJump};
    conditionBlock.successors = {0x3000, 0x4000};

    Disasm::BasicBlock trueBlock;
    trueBlock.startAddress = 0x3000;
    trueBlock.instructions = {ReturnInstruction(0x3000)};
    trueBlock.endsWithReturn = true;

    Disasm::BasicBlock falseBlock;
    falseBlock.startAddress = 0x4000;
    falseBlock.instructions = {ReturnInstruction(0x4000)};
    falseBlock.endsWithReturn = true;

    Disasm::ControlFlowGraph graph;
    graph.name = "CrossBlockFlags";
    graph.entryAddress = comparisonBlock.startAddress;
    graph.blocks = {comparisonBlock, conditionBlock, trueBlock, falseBlock};

    CHECK(LiftFailure(graph) == "lift: condition depends on flags from another basic block");
}

TEST_CASE("lifting rejects carry conditions without complete flag semantics", "[lift]")
{
    Disasm::DecodedInstruction add;
    add.address = 0x1000;
    add.mnemonic = Disasm::Mnemonic::Add;
    add.operands = {RegisterOperand(Disasm::GpRegister::Rax), ImmediateOperand(1)};

    Disasm::DecodedInstruction conditionalJump;
    conditionalJump.address = 0x1001;
    conditionalJump.mnemonic = Disasm::Mnemonic::ConditionalJump;
    conditionalJump.condition = Disasm::Condition::Below;

    Disasm::BasicBlock conditionBlock;
    conditionBlock.startAddress = 0x1000;
    conditionBlock.instructions = {add, conditionalJump};
    conditionBlock.successors = {0x2000, 0x3000};

    Disasm::BasicBlock trueBlock;
    trueBlock.startAddress = 0x2000;
    trueBlock.instructions = {ReturnInstruction(0x2000)};
    trueBlock.endsWithReturn = true;

    Disasm::BasicBlock falseBlock;
    falseBlock.startAddress = 0x3000;
    falseBlock.instructions = {ReturnInstruction(0x3000)};
    falseBlock.endsWithReturn = true;

    Disasm::ControlFlowGraph graph;
    graph.name = "CarryCondition";
    graph.entryAddress = conditionBlock.startAddress;
    graph.blocks = {conditionBlock, trueBlock, falseBlock};

    CHECK(LiftFailure(graph) == "lift: condition requires unsupported flag semantics");
}

TEST_CASE("lifting rejects carry conditions after rotate", "[lift]")
{
    Disasm::DecodedInstruction compare;
    compare.address = 0x1000;
    compare.mnemonic = Disasm::Mnemonic::Cmp;
    compare.operands = {RegisterOperand(Disasm::GpRegister::Rax), ImmediateOperand(0)};

    Disasm::DecodedInstruction rotate;
    rotate.address = 0x1001;
    rotate.mnemonic = Disasm::Mnemonic::Rol;
    rotate.operands = {RegisterOperand(Disasm::GpRegister::Rax), ImmediateOperand(1)};

    Disasm::DecodedInstruction conditionalJump;
    conditionalJump.address = 0x1002;
    conditionalJump.mnemonic = Disasm::Mnemonic::ConditionalJump;
    conditionalJump.condition = Disasm::Condition::Below;

    Disasm::BasicBlock conditionBlock;
    conditionBlock.startAddress = 0x1000;
    conditionBlock.instructions = {compare, rotate, conditionalJump};
    conditionBlock.successors = {0x2000, 0x3000};

    Disasm::BasicBlock trueBlock;
    trueBlock.startAddress = 0x2000;
    trueBlock.instructions = {ReturnInstruction(0x2000)};
    trueBlock.endsWithReturn = true;

    Disasm::BasicBlock falseBlock;
    falseBlock.startAddress = 0x3000;
    falseBlock.instructions = {ReturnInstruction(0x3000)};
    falseBlock.endsWithReturn = true;

    Disasm::ControlFlowGraph graph;
    graph.name = "RotateCarryCondition";
    graph.entryAddress = conditionBlock.startAddress;
    graph.blocks = {conditionBlock, trueBlock, falseBlock};

    CHECK(LiftFailure(graph) == "lift: condition requires unsupported flag semantics");
}
