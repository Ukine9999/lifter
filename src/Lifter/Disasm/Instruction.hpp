#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Lifter::Disasm
{
    enum class GpRegister : std::uint32_t
    {
        Rax,
        Rcx,
        Rdx,
        Rbx,
        Rsp,
        Rbp,
        Rsi,
        Rdi,
        R8,
        R9,
        R10,
        R11,
        R12,
        R13,
        R14,
        R15,
        None,
    };

    enum class Mnemonic : std::uint32_t
    {
        Mov,
        Movzx,
        Movsx,
        Lea,
        Add,
        Sub,
        Inc,
        Dec,
        Neg,
        Xor,
        Or,
        And,
        Not,
        Imul,
        Mul,
        Shl,
        Shr,
        Sar,
        Rol,
        Ror,
        Cmp,
        Test,
        Jmp,
        ConditionalJump,
        SetConditional,
        Push,
        Pop,
        Ret,
        Nop,
        Unsupported,
    };

    enum class Condition : std::uint32_t
    {
        Equal,
        NotEqual,
        Less,
        GreaterEqual,
        Greater,
        LessEqual,
        Below,
        AboveEqual,
        Above,
        BelowEqual,
        Sign,
        NotSign,
        None,
    };

    enum class OperandKind : std::uint32_t
    {
        Register,
        Immediate,
        Memory,
    };

    struct RegisterOperand
    {
        GpRegister reg = GpRegister::None;
        unsigned widthBits = 0;
        bool isHigh8 = false;
    };

    struct ImmediateOperand
    {
        std::uint64_t value = 0;
        unsigned widthBits = 0;
    };

    struct MemoryOperand
    {
        GpRegister base = GpRegister::None;
        GpRegister index = GpRegister::None;
        unsigned scale = 1;
        std::int64_t displacement = 0;
        unsigned widthBits = 0;
        bool isInstructionPointerRelative = false;
    };

    struct Operand
    {
        OperandKind kind = OperandKind::Register;
        RegisterOperand reg;
        ImmediateOperand immediate;
        MemoryOperand memory;
    };

    struct DecodedInstruction
    {
        std::uint64_t address = 0;
        unsigned length = 0;
        Mnemonic mnemonic = Mnemonic::Unsupported;
        Condition condition = Condition::None;
        std::vector<Operand> operands;
        std::uint64_t branchTarget = 0;
        bool hasBranchTarget = false;
        std::string text;
        std::vector<std::uint8_t> bytes;
    };
}
