#include "Lifter/Disasm/ZydisDisassembler.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

#include <Zydis/Zydis.h>

namespace Lifter::Disasm
{
    namespace
    {
        GpRegister MapEnclosingRegister(ZydisRegister enclosing)
        {
            switch (enclosing)
            {
                case ZYDIS_REGISTER_RAX:
                    return GpRegister::Rax;
                case ZYDIS_REGISTER_RCX:
                    return GpRegister::Rcx;
                case ZYDIS_REGISTER_RDX:
                    return GpRegister::Rdx;
                case ZYDIS_REGISTER_RBX:
                    return GpRegister::Rbx;
                case ZYDIS_REGISTER_RSP:
                    return GpRegister::Rsp;
                case ZYDIS_REGISTER_RBP:
                    return GpRegister::Rbp;
                case ZYDIS_REGISTER_RSI:
                    return GpRegister::Rsi;
                case ZYDIS_REGISTER_RDI:
                    return GpRegister::Rdi;
                case ZYDIS_REGISTER_R8:
                    return GpRegister::R8;
                case ZYDIS_REGISTER_R9:
                    return GpRegister::R9;
                case ZYDIS_REGISTER_R10:
                    return GpRegister::R10;
                case ZYDIS_REGISTER_R11:
                    return GpRegister::R11;
                case ZYDIS_REGISTER_R12:
                    return GpRegister::R12;
                case ZYDIS_REGISTER_R13:
                    return GpRegister::R13;
                case ZYDIS_REGISTER_R14:
                    return GpRegister::R14;
                case ZYDIS_REGISTER_R15:
                    return GpRegister::R15;
                default:
                    return GpRegister::None;
            }
        }

        GpRegister MapRegister(ZydisRegister reg)
        {
            if (reg == ZYDIS_REGISTER_NONE) return GpRegister::None;

            const ZydisRegister enclosing = ZydisRegisterGetLargestEnclosing(ZYDIS_MACHINE_MODE_LONG_64, reg);
            return MapEnclosingRegister(enclosing);
        }

        RegisterOperand MakeRegisterOperand(ZydisRegister reg)
        {
            RegisterOperand result;
            result.reg = MapRegister(reg);
            const ZydisRegisterWidth width = ZydisRegisterGetWidth(ZYDIS_MACHINE_MODE_LONG_64, reg);
            result.widthBits = static_cast<unsigned>(width);
            result.isHigh8 = reg == ZYDIS_REGISTER_AH || reg == ZYDIS_REGISTER_BH || reg == ZYDIS_REGISTER_CH ||
                             reg == ZYDIS_REGISTER_DH;
            return result;
        }

        bool MapMnemonic(ZydisMnemonic zydisMnemonic, Mnemonic& mnemonic, Condition& condition)
        {
            condition = Condition::None;

            switch (zydisMnemonic)
            {
                case ZYDIS_MNEMONIC_MOV:
                    mnemonic = Mnemonic::Mov;
                    return true;
                case ZYDIS_MNEMONIC_MOVZX:
                    mnemonic = Mnemonic::Movzx;
                    return true;
                case ZYDIS_MNEMONIC_MOVSX:
                    mnemonic = Mnemonic::Movsx;
                    return true;
                case ZYDIS_MNEMONIC_MOVSXD:
                    mnemonic = Mnemonic::Movsx;
                    return true;
                case ZYDIS_MNEMONIC_LEA:
                    mnemonic = Mnemonic::Lea;
                    return true;
                case ZYDIS_MNEMONIC_ADD:
                    mnemonic = Mnemonic::Add;
                    return true;
                case ZYDIS_MNEMONIC_SUB:
                    mnemonic = Mnemonic::Sub;
                    return true;
                case ZYDIS_MNEMONIC_INC:
                    mnemonic = Mnemonic::Inc;
                    return true;
                case ZYDIS_MNEMONIC_DEC:
                    mnemonic = Mnemonic::Dec;
                    return true;
                case ZYDIS_MNEMONIC_NEG:
                    mnemonic = Mnemonic::Neg;
                    return true;
                case ZYDIS_MNEMONIC_XOR:
                    mnemonic = Mnemonic::Xor;
                    return true;
                case ZYDIS_MNEMONIC_OR:
                    mnemonic = Mnemonic::Or;
                    return true;
                case ZYDIS_MNEMONIC_AND:
                    mnemonic = Mnemonic::And;
                    return true;
                case ZYDIS_MNEMONIC_NOT:
                    mnemonic = Mnemonic::Not;
                    return true;
                case ZYDIS_MNEMONIC_IMUL:
                    mnemonic = Mnemonic::Imul;
                    return true;
                case ZYDIS_MNEMONIC_MUL:
                    mnemonic = Mnemonic::Mul;
                    return true;
                case ZYDIS_MNEMONIC_SHL:
                    mnemonic = Mnemonic::Shl;
                    return true;
                case ZYDIS_MNEMONIC_SHR:
                    mnemonic = Mnemonic::Shr;
                    return true;
                case ZYDIS_MNEMONIC_SAR:
                    mnemonic = Mnemonic::Sar;
                    return true;
                case ZYDIS_MNEMONIC_ROL:
                    mnemonic = Mnemonic::Rol;
                    return true;
                case ZYDIS_MNEMONIC_ROR:
                    mnemonic = Mnemonic::Ror;
                    return true;
                case ZYDIS_MNEMONIC_CMP:
                    mnemonic = Mnemonic::Cmp;
                    return true;
                case ZYDIS_MNEMONIC_TEST:
                    mnemonic = Mnemonic::Test;
                    return true;
                case ZYDIS_MNEMONIC_JMP:
                    mnemonic = Mnemonic::Jmp;
                    return true;
                case ZYDIS_MNEMONIC_PUSH:
                    mnemonic = Mnemonic::Push;
                    return true;
                case ZYDIS_MNEMONIC_POP:
                    mnemonic = Mnemonic::Pop;
                    return true;
                case ZYDIS_MNEMONIC_RET:
                    mnemonic = Mnemonic::Ret;
                    return true;
                case ZYDIS_MNEMONIC_NOP:
                    mnemonic = Mnemonic::Nop;
                    return true;
                case ZYDIS_MNEMONIC_JZ:
                    mnemonic = Mnemonic::ConditionalJump;
                    condition = Condition::Equal;
                    return true;
                case ZYDIS_MNEMONIC_JNZ:
                    mnemonic = Mnemonic::ConditionalJump;
                    condition = Condition::NotEqual;
                    return true;
                case ZYDIS_MNEMONIC_JL:
                    mnemonic = Mnemonic::ConditionalJump;
                    condition = Condition::Less;
                    return true;
                case ZYDIS_MNEMONIC_JNL:
                    mnemonic = Mnemonic::ConditionalJump;
                    condition = Condition::GreaterEqual;
                    return true;
                case ZYDIS_MNEMONIC_JLE:
                    mnemonic = Mnemonic::ConditionalJump;
                    condition = Condition::LessEqual;
                    return true;
                case ZYDIS_MNEMONIC_JNLE:
                    mnemonic = Mnemonic::ConditionalJump;
                    condition = Condition::Greater;
                    return true;
                case ZYDIS_MNEMONIC_JB:
                    mnemonic = Mnemonic::ConditionalJump;
                    condition = Condition::Below;
                    return true;
                case ZYDIS_MNEMONIC_JNB:
                    mnemonic = Mnemonic::ConditionalJump;
                    condition = Condition::AboveEqual;
                    return true;
                case ZYDIS_MNEMONIC_JBE:
                    mnemonic = Mnemonic::ConditionalJump;
                    condition = Condition::BelowEqual;
                    return true;
                case ZYDIS_MNEMONIC_JNBE:
                    mnemonic = Mnemonic::ConditionalJump;
                    condition = Condition::Above;
                    return true;
                case ZYDIS_MNEMONIC_JS:
                    mnemonic = Mnemonic::ConditionalJump;
                    condition = Condition::Sign;
                    return true;
                case ZYDIS_MNEMONIC_JNS:
                    mnemonic = Mnemonic::ConditionalJump;
                    condition = Condition::NotSign;
                    return true;
                case ZYDIS_MNEMONIC_SETZ:
                    mnemonic = Mnemonic::SetConditional;
                    condition = Condition::Equal;
                    return true;
                case ZYDIS_MNEMONIC_SETNZ:
                    mnemonic = Mnemonic::SetConditional;
                    condition = Condition::NotEqual;
                    return true;
                case ZYDIS_MNEMONIC_SETL:
                    mnemonic = Mnemonic::SetConditional;
                    condition = Condition::Less;
                    return true;
                case ZYDIS_MNEMONIC_SETNLE:
                    mnemonic = Mnemonic::SetConditional;
                    condition = Condition::Greater;
                    return true;
                default:
                    mnemonic = Mnemonic::Unsupported;
                    return false;
            }
        }

        Operand TranslateOperand(const ZydisDecodedOperand& source)
        {
            Operand operand;

            if (source.type == ZYDIS_OPERAND_TYPE_REGISTER)
            {
                operand.kind = OperandKind::Register;
                operand.reg = MakeRegisterOperand(source.reg.value);
            }
            else if (source.type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
            {
                operand.kind = OperandKind::Immediate;
                operand.immediate.value = source.imm.value.u;
                operand.immediate.widthBits = source.size;
            }
            else if (source.type == ZYDIS_OPERAND_TYPE_MEMORY)
            {
                operand.kind = OperandKind::Memory;
                operand.memory.base = MapRegister(source.mem.base);
                operand.memory.index = MapRegister(source.mem.index);
                operand.memory.scale = source.mem.scale;
                operand.memory.displacement = source.mem.disp.value;
                operand.memory.widthBits = source.size;
                operand.memory.isInstructionPointerRelative =
                    source.mem.base == ZYDIS_REGISTER_RIP || source.mem.base == ZYDIS_REGISTER_EIP;
            }

            return operand;
        }
    }

    ControlFlowGraph ZydisDisassembler::DisassembleFunction(const AddressSpace::IAddressSpace& image,
                                                            std::uint64_t entryAddress, const std::string& name) const
    {
        ZydisDecoder decoder;
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

        ZydisFormatter formatter;
        ZydisFormatterInit(&formatter, ZYDIS_FORMATTER_STYLE_INTEL);

        std::map<std::uint64_t, DecodedInstruction> decoded;
        std::set<std::uint64_t> leaders;
        std::vector<std::uint64_t> worklist{entryAddress};
        leaders.insert(entryAddress);

        while (!worklist.empty())
        {
            const std::uint64_t address = worklist.back();
            worklist.pop_back();

            if (decoded.count(address) != 0) continue;

            const std::uint64_t available = image.AvailableBytesFrom(address);
            if (available == 0) throw std::runtime_error("disasm: address outside mapped image");

            const std::uint64_t window = std::min<std::uint64_t>(available, ZYDIS_MAX_INSTRUCTION_LENGTH);
            const std::uint8_t* bytes = image.PointerTo(address, window);

            ZydisDecodedInstruction zydisInstruction;
            ZydisDecodedOperand zydisOperands[ZYDIS_MAX_OPERAND_COUNT];
            if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, bytes, window, &zydisInstruction, zydisOperands)))
                throw std::runtime_error("disasm: failed to decode instruction");

            DecodedInstruction instruction;
            instruction.address = address;
            instruction.length = zydisInstruction.length;
            instruction.bytes.assign(bytes, bytes + zydisInstruction.length);

            MapMnemonic(zydisInstruction.mnemonic, instruction.mnemonic, instruction.condition);

            char textBuffer[128];
            ZydisFormatterFormatInstruction(&formatter, &zydisInstruction, zydisOperands,
                                            zydisInstruction.operand_count_visible, textBuffer, sizeof(textBuffer),
                                            address, ZYAN_NULL);
            instruction.text = textBuffer;

            for (std::uint8_t operandIndex = 0; operandIndex < zydisInstruction.operand_count_visible; ++operandIndex)
            {
                const ZydisDecodedOperand& source = zydisOperands[operandIndex];

                if (source.type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
                    (instruction.mnemonic == Mnemonic::Jmp || instruction.mnemonic == Mnemonic::ConditionalJump))
                {
                    ZyanU64 target = 0;
                    ZydisCalcAbsoluteAddress(&zydisInstruction, &source, address, &target);
                    instruction.branchTarget = target;
                    instruction.hasBranchTarget = true;
                    continue;
                }

                instruction.operands.push_back(TranslateOperand(source));
            }

            const std::uint64_t nextAddress = address + instruction.length;
            decoded.emplace(address, instruction);

            if (instruction.mnemonic == Mnemonic::Ret) continue;

            if (instruction.mnemonic == Mnemonic::Jmp)
            {
                if (instruction.hasBranchTarget)
                {
                    leaders.insert(instruction.branchTarget);
                    worklist.push_back(instruction.branchTarget);
                }
                continue;
            }

            if (instruction.mnemonic == Mnemonic::ConditionalJump)
            {
                leaders.insert(instruction.branchTarget);
                leaders.insert(nextAddress);
                worklist.push_back(instruction.branchTarget);
                worklist.push_back(nextAddress);
                continue;
            }

            worklist.push_back(nextAddress);
        }

        ControlFlowGraph graph;
        graph.name = name;
        graph.entryAddress = entryAddress;

        for (std::uint64_t blockStart : leaders)
        {
            if (decoded.count(blockStart) == 0) continue;

            BasicBlock block;
            block.startAddress = blockStart;

            std::uint64_t address = blockStart;
            while (true)
            {
                const DecodedInstruction& instruction = decoded.at(address);
                block.instructions.push_back(instruction);
                const std::uint64_t nextAddress = address + instruction.length;

                if (instruction.mnemonic == Mnemonic::Ret)
                {
                    block.endsWithReturn = true;
                    break;
                }

                if (instruction.mnemonic == Mnemonic::Jmp)
                {
                    block.successors.push_back(instruction.branchTarget);
                    break;
                }

                if (instruction.mnemonic == Mnemonic::ConditionalJump)
                {
                    block.successors.push_back(instruction.branchTarget);
                    block.successors.push_back(nextAddress);
                    break;
                }

                if (leaders.count(nextAddress) != 0 || decoded.count(nextAddress) == 0)
                {
                    block.successors.push_back(nextAddress);
                    break;
                }

                address = nextAddress;
            }

            graph.blocks.push_back(std::move(block));
        }

        std::sort(graph.blocks.begin(), graph.blocks.end(), [](const BasicBlock& left, const BasicBlock& right)
                  { return left.startAddress < right.startAddress; });

        return graph;
    }
}
