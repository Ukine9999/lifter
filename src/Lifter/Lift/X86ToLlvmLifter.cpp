#include "Lifter/Lift/X86ToLlvmLifter.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

#include "Lifter/Ir/OwnedLlvmModule.hpp"

namespace Lifter::Lift
{
    using namespace Lifter::Disasm;

    namespace
    {
        std::string RegisterName(GpRegister reg)
        {
            switch (reg)
            {
                case GpRegister::Rax:
                    return "rax";
                case GpRegister::Rcx:
                    return "rcx";
                case GpRegister::Rdx:
                    return "rdx";
                case GpRegister::Rbx:
                    return "rbx";
                case GpRegister::Rsp:
                    return "rsp";
                case GpRegister::Rbp:
                    return "rbp";
                case GpRegister::Rsi:
                    return "rsi";
                case GpRegister::Rdi:
                    return "rdi";
                case GpRegister::R8:
                    return "r8";
                case GpRegister::R9:
                    return "r9";
                case GpRegister::R10:
                    return "r10";
                case GpRegister::R11:
                    return "r11";
                case GpRegister::R12:
                    return "r12";
                case GpRegister::R13:
                    return "r13";
                case GpRegister::R14:
                    return "r14";
                case GpRegister::R15:
                    return "r15";
                default:
                    return "none";
            }
        }

        std::array<GpRegister, 16> AllRegisters()
        {
            return {GpRegister::Rax, GpRegister::Rcx, GpRegister::Rdx, GpRegister::Rbx,
                    GpRegister::Rsp, GpRegister::Rbp, GpRegister::Rsi, GpRegister::Rdi,
                    GpRegister::R8,  GpRegister::R9,  GpRegister::R10, GpRegister::R11,
                    GpRegister::R12, GpRegister::R13, GpRegister::R14, GpRegister::R15};
        }

        bool IsStackRegister(const Operand& operand)
        {
            return operand.kind == OperandKind::Register &&
                   (operand.reg.reg == GpRegister::Rsp || operand.reg.reg == GpRegister::Rbp);
        }

        bool UsesStackRegister(const MemoryOperand& memory)
        {
            return memory.base == GpRegister::Rsp || memory.base == GpRegister::Rbp ||
                   memory.index == GpRegister::Rsp || memory.index == GpRegister::Rbp;
        }

        class FunctionLifter
        {
        public:
            FunctionLifter(llvm::LLVMContext& context, llvm::Module& module, const ControlFlowGraph& graph)
                : m_context(context), m_module(module), m_graph(graph), m_builder(context),
                  m_int64(llvm::Type::getInt64Ty(context))
            {
            }

            void Lift()
            {
                ValidateGraph();

                llvm::Type* argumentTypes[] = {m_int64, m_int64, m_int64, m_int64};
                llvm::FunctionType* functionType = llvm::FunctionType::get(m_int64, argumentTypes, false);
                m_function =
                    llvm::Function::Create(functionType, llvm::Function::ExternalLinkage, m_graph.name, m_module);

                llvm::BasicBlock* entry = llvm::BasicBlock::Create(m_context, "entry", m_function);

                for (const BasicBlock& block : m_graph.blocks)
                    m_blocks[block.startAddress] =
                        llvm::BasicBlock::Create(m_context, "b" + Hex(block.startAddress), m_function);

                m_builder.SetInsertPoint(entry);
                EmitPrologue();
                m_builder.CreateBr(BlockAt(m_graph.entryAddress));

                for (const BasicBlock& block : m_graph.blocks)
                    EmitBlock(block);
            }

        private:
            void ValidateGraph() const
            {
                for (const BasicBlock& block : m_graph.blocks)
                    for (const DecodedInstruction& instruction : block.instructions)
                    {
                        for (const Operand& operand : instruction.operands)
                        {
                            if (operand.kind == OperandKind::Memory && operand.memory.isInstructionPointerRelative)
                                throw std::runtime_error("lift: instruction-pointer-relative memory is not supported");
                        }

                        const bool writesFirstOperand =
                            instruction.mnemonic == Mnemonic::Mov || instruction.mnemonic == Mnemonic::Movzx ||
                            instruction.mnemonic == Mnemonic::Movsx || instruction.mnemonic == Mnemonic::Add ||
                            instruction.mnemonic == Mnemonic::Sub || instruction.mnemonic == Mnemonic::Imul ||
                            instruction.mnemonic == Mnemonic::Xor || instruction.mnemonic == Mnemonic::Or ||
                            instruction.mnemonic == Mnemonic::And;
                        if (writesFirstOperand && instruction.operands.size() >= 2 &&
                            IsStackRegister(instruction.operands[1]) && !IsStackRegister(instruction.operands[0]))
                            throw std::runtime_error("lift: derived synthetic-stack addresses are not supported");

                        if (instruction.mnemonic == Mnemonic::Lea && instruction.operands.size() >= 2 &&
                            UsesStackRegister(instruction.operands[1].memory) &&
                            !IsStackRegister(instruction.operands[0]))
                            throw std::runtime_error("lift: derived synthetic-stack addresses are not supported");

                        if (instruction.mnemonic == Mnemonic::Push && !instruction.operands.empty() &&
                            IsStackRegister(instruction.operands[0]))
                            throw std::runtime_error("lift: derived synthetic-stack addresses are not supported");

                        if (instruction.mnemonic == Mnemonic::Pop && !instruction.operands.empty() &&
                            IsStackRegister(instruction.operands[0]))
                            throw std::runtime_error("lift: pop into a stack register is not supported");
                    }
            }

            static std::string Hex(std::uint64_t value)
            {
                char buffer[20];
                std::snprintf(buffer, sizeof(buffer), "%llx", static_cast<unsigned long long>(value));
                return buffer;
            }

            llvm::Type* IntegerType(unsigned widthBits)
            {
                return llvm::Type::getIntNTy(m_context, widthBits);
            }

            llvm::Constant* Constant64(std::uint64_t value)
            {
                return llvm::ConstantInt::get(m_int64, value);
            }

            llvm::BasicBlock* BlockAt(std::uint64_t address)
            {
                const auto found = m_blocks.find(address);
                if (found == m_blocks.end())
                    throw std::runtime_error("lift: branch to unknown block 0x" + Hex(address));
                return found->second;
            }

            void EmitPrologue()
            {
                for (const GpRegister reg : AllRegisters())
                    m_slots[reg] = m_builder.CreateAlloca(m_int64, nullptr, "reg_" + RegisterName(reg));

                llvm::AllocaInst* stack =
                    m_builder.CreateAlloca(llvm::ArrayType::get(IntegerType(8), 1024), nullptr, "stack");

                for (const GpRegister reg : AllRegisters())
                    m_builder.CreateStore(Constant64(0), m_slots[reg]);

                llvm::Value* stackInteger = m_builder.CreatePtrToInt(stack, m_int64);
                m_stackStart = stackInteger;
                m_stackEnd = m_builder.CreateAdd(stackInteger, Constant64(1024));
                m_builder.CreateStore(m_builder.CreateAdd(stackInteger, Constant64(512)), m_slots[GpRegister::Rsp]);

                m_builder.CreateStore(m_function->getArg(0), m_slots[GpRegister::Rcx]);
                m_builder.CreateStore(m_function->getArg(1), m_slots[GpRegister::Rdx]);
                m_builder.CreateStore(m_function->getArg(2), m_slots[GpRegister::R8]);
                m_builder.CreateStore(m_function->getArg(3), m_slots[GpRegister::R9]);
            }

            llvm::Value* ReadRegister(GpRegister reg, unsigned widthBits, bool isHigh8)
            {
                llvm::Value* value = m_builder.CreateLoad(m_int64, m_slots[reg]);

                if (isHigh8)
                {
                    value = m_builder.CreateLShr(value, Constant64(8));
                    widthBits = 8;
                }

                if (widthBits >= 64) return value;

                const std::uint64_t mask = (widthBits == 32) ? 0xFFFFFFFFull : ((1ull << widthBits) - 1);
                return m_builder.CreateAnd(value, Constant64(mask));
            }

            void WriteRegister(GpRegister reg, unsigned widthBits, bool isHigh8, llvm::Value* value)
            {
                if (widthBits >= 64)
                {
                    m_builder.CreateStore(value, m_slots[reg]);
                    return;
                }

                if (widthBits == 32 && !isHigh8)
                {
                    m_builder.CreateStore(m_builder.CreateAnd(value, Constant64(0xFFFFFFFFull)), m_slots[reg]);
                    return;
                }

                llvm::Value* old = m_builder.CreateLoad(m_int64, m_slots[reg]);
                const std::uint64_t mask = isHigh8 ? 0xFF00ull : ((1ull << widthBits) - 1);
                llvm::Value* cleared = m_builder.CreateAnd(old, Constant64(~mask));
                llvm::Value* contribution = isHigh8 ? m_builder.CreateShl(value, Constant64(8)) : value;
                llvm::Value* constrained = m_builder.CreateAnd(contribution, Constant64(mask));
                m_builder.CreateStore(m_builder.CreateOr(cleared, constrained), m_slots[reg]);
            }

            llvm::Value* ComputeAddress(const MemoryOperand& memory)
            {
                llvm::Value* address = (memory.base != GpRegister::None) ? ReadRegister(memory.base, 64, false)
                                                                         : static_cast<llvm::Value*>(Constant64(0));

                if (memory.index != GpRegister::None)
                {
                    llvm::Value* indexValue = ReadRegister(memory.index, 64, false);
                    if (memory.scale > 1) indexValue = m_builder.CreateMul(indexValue, Constant64(memory.scale));
                    address = m_builder.CreateAdd(address, indexValue);
                }

                if (memory.displacement != 0)
                    address = m_builder.CreateAdd(address, Constant64(static_cast<std::uint64_t>(memory.displacement)));

                return address;
            }

            static bool UsesSyntheticStack(const MemoryOperand& memory)
            {
                return UsesStackRegister(memory);
            }

            void GuardStackAccess(llvm::Value* address, unsigned widthBits)
            {
                const std::uint64_t byteCount = (widthBits + 7u) / 8u;
                llvm::Value* afterLastValidStart = m_builder.CreateSub(m_stackEnd, Constant64(byteCount));
                llvm::Value* startsInStack = m_builder.CreateICmpUGE(address, m_stackStart);
                llvm::Value* endsInStack = m_builder.CreateICmpULE(address, afterLastValidStart);
                llvm::Value* inBounds = m_builder.CreateAnd(startsInStack, endsInStack);

                llvm::BasicBlock* valid = llvm::BasicBlock::Create(m_context, "stack.valid", m_function);
                llvm::BasicBlock* invalid = llvm::BasicBlock::Create(m_context, "stack.invalid", m_function);
                m_builder.CreateCondBr(inBounds, valid, invalid);

                m_builder.SetInsertPoint(invalid);
                llvm::Function* trap = llvm::Intrinsic::getDeclaration(&m_module, llvm::Intrinsic::trap);
                m_builder.CreateCall(trap);
                m_builder.CreateUnreachable();

                m_builder.SetInsertPoint(valid);
            }

            llvm::Value* LoadMemory(llvm::Value* address, unsigned widthBits, bool guardStack = false)
            {
                if (guardStack) GuardStackAccess(address, widthBits);

                llvm::Type* accessType = IntegerType(widthBits);
                llvm::Value* pointer = m_builder.CreateIntToPtr(address, m_builder.getPtrTy());
                llvm::Value* value = m_builder.CreateLoad(accessType, pointer);

                if (widthBits >= 64) return value;

                return m_builder.CreateZExt(value, m_int64);
            }

            void StoreMemory(llvm::Value* address, unsigned widthBits, llvm::Value* value, bool guardStack = false)
            {
                if (guardStack) GuardStackAccess(address, widthBits);

                llvm::Type* accessType = IntegerType(widthBits);
                llvm::Value* pointer = m_builder.CreateIntToPtr(address, m_builder.getPtrTy());
                llvm::Value* stored = (widthBits >= 64) ? value : m_builder.CreateTrunc(value, accessType);
                m_builder.CreateStore(stored, pointer);
            }

            unsigned OperandWidth(const Operand& operand)
            {
                if (operand.kind == OperandKind::Register) return operand.reg.widthBits;
                if (operand.kind == OperandKind::Memory) return operand.memory.widthBits;
                return operand.immediate.widthBits;
            }

            llvm::Value* ReadOperand(const Operand& operand)
            {
                if (operand.kind == OperandKind::Register)
                    return ReadRegister(operand.reg.reg, operand.reg.widthBits, operand.reg.isHigh8);
                if (operand.kind == OperandKind::Immediate) return Constant64(operand.immediate.value);
                return LoadMemory(ComputeAddress(operand.memory), operand.memory.widthBits,
                                  UsesSyntheticStack(operand.memory));
            }

            void WriteOperand(const Operand& operand, llvm::Value* value)
            {
                if (operand.kind == OperandKind::Register)
                    WriteRegister(operand.reg.reg, operand.reg.widthBits, operand.reg.isHigh8, value);
                else if (operand.kind == OperandKind::Memory)
                    StoreMemory(ComputeAddress(operand.memory), operand.memory.widthBits, value,
                                UsesSyntheticStack(operand.memory));
            }

            void SetComparisonFlags(llvm::Value* left, llvm::Value* right, unsigned widthBits)
            {
                m_flagLeft = left;
                m_flagRight = right;
                m_flagWidth = widthBits;
                m_flagsFromComparison = true;
                m_carryConditionsSupported = true;
            }

            void SetResultFlags(llvm::Value* result, unsigned widthBits)
            {
                m_flagLeft = result;
                m_flagRight = Constant64(0);
                m_flagWidth = widthBits;
                m_flagsFromComparison = false;
                m_carryConditionsSupported = false;
            }

            llvm::Value* EmitCondition(Condition condition)
            {
                if (m_flagLeft == nullptr || m_flagRight == nullptr)
                    throw std::runtime_error("lift: condition depends on flags from another basic block");

                const bool resultCondition = condition == Condition::Equal || condition == Condition::NotEqual ||
                                             condition == Condition::Sign || condition == Condition::NotSign;
                const bool carryCondition = condition == Condition::Below || condition == Condition::AboveEqual ||
                                            condition == Condition::Above || condition == Condition::BelowEqual;
                const bool comparisonCondition = condition == Condition::Equal || condition == Condition::NotEqual ||
                                                 condition == Condition::Less || condition == Condition::GreaterEqual ||
                                                 condition == Condition::Greater || condition == Condition::LessEqual ||
                                                 condition == Condition::Below || condition == Condition::AboveEqual ||
                                                 condition == Condition::Above || condition == Condition::BelowEqual;
                if (carryCondition && !m_carryConditionsSupported)
                    throw std::runtime_error("lift: condition requires unsupported flag semantics");
                if ((!m_flagsFromComparison && !resultCondition) || (m_flagsFromComparison && !comparisonCondition))
                    throw std::runtime_error("lift: condition requires unsupported flag semantics");

                const unsigned width = m_flagWidth;
                llvm::Value* left = m_flagLeft;
                llvm::Value* right = m_flagRight;

                if (width < 64)
                {
                    left = m_builder.CreateTrunc(left, IntegerType(width));
                    right = m_builder.CreateTrunc(right, IntegerType(width));
                }

                switch (condition)
                {
                    case Condition::Equal:
                        return m_builder.CreateICmpEQ(left, right);
                    case Condition::NotEqual:
                        return m_builder.CreateICmpNE(left, right);
                    case Condition::Less:
                        return m_builder.CreateICmpSLT(left, right);
                    case Condition::GreaterEqual:
                        return m_builder.CreateICmpSGE(left, right);
                    case Condition::Greater:
                        return m_builder.CreateICmpSGT(left, right);
                    case Condition::LessEqual:
                        return m_builder.CreateICmpSLE(left, right);
                    case Condition::Below:
                        return m_builder.CreateICmpULT(left, right);
                    case Condition::AboveEqual:
                        return m_builder.CreateICmpUGE(left, right);
                    case Condition::Above:
                        return m_builder.CreateICmpUGT(left, right);
                    case Condition::BelowEqual:
                        return m_builder.CreateICmpULE(left, right);
                    case Condition::Sign:
                        return m_builder.CreateICmpSLT(left, right);
                    case Condition::NotSign:
                        return m_builder.CreateICmpSGE(left, right);
                    default:
                        return m_builder.CreateICmpEQ(left, right);
                }
            }

            llvm::Value* ShiftCount(const Operand& source, unsigned widthBits)
            {
                llvm::Value* count = ReadOperand(source);
                const std::uint64_t mask = (widthBits == 64) ? 63u : 31u;
                return m_builder.CreateAnd(count, Constant64(mask));
            }

            void EmitInstruction(const DecodedInstruction& instruction, const BasicBlock& block)
            {
                switch (instruction.mnemonic)
                {
                    case Mnemonic::Mov:
                    case Mnemonic::Movzx:
                        WriteOperand(instruction.operands[0], ReadOperand(instruction.operands[1]));
                        return;

                    case Mnemonic::Movsx:
                    {
                        const unsigned sourceWidth = OperandWidth(instruction.operands[1]);
                        llvm::Value* narrow =
                            m_builder.CreateTrunc(ReadOperand(instruction.operands[1]), IntegerType(sourceWidth));
                        WriteOperand(instruction.operands[0], m_builder.CreateSExt(narrow, m_int64));
                        return;
                    }

                    case Mnemonic::Lea:
                        WriteOperand(instruction.operands[0], ComputeAddress(instruction.operands[1].memory));
                        return;

                    case Mnemonic::Add:
                    {
                        llvm::Value* result = m_builder.CreateAdd(ReadOperand(instruction.operands[0]),
                                                                  ReadOperand(instruction.operands[1]));
                        WriteOperand(instruction.operands[0], result);
                        SetResultFlags(result, OperandWidth(instruction.operands[0]));
                        return;
                    }

                    case Mnemonic::Sub:
                    {
                        llvm::Value* left = ReadOperand(instruction.operands[0]);
                        llvm::Value* right = ReadOperand(instruction.operands[1]);
                        WriteOperand(instruction.operands[0], m_builder.CreateSub(left, right));
                        SetComparisonFlags(left, right, OperandWidth(instruction.operands[0]));
                        return;
                    }

                    case Mnemonic::Imul:
                    {
                        llvm::Value* result = m_builder.CreateMul(ReadOperand(instruction.operands[0]),
                                                                  ReadOperand(instruction.operands[1]));
                        WriteOperand(instruction.operands[0], result);
                        SetResultFlags(result, OperandWidth(instruction.operands[0]));
                        return;
                    }

                    case Mnemonic::Xor:
                    {
                        llvm::Value* result = m_builder.CreateXor(ReadOperand(instruction.operands[0]),
                                                                  ReadOperand(instruction.operands[1]));
                        WriteOperand(instruction.operands[0], result);
                        SetResultFlags(result, OperandWidth(instruction.operands[0]));
                        return;
                    }

                    case Mnemonic::Or:
                    {
                        llvm::Value* result = m_builder.CreateOr(ReadOperand(instruction.operands[0]),
                                                                 ReadOperand(instruction.operands[1]));
                        WriteOperand(instruction.operands[0], result);
                        SetResultFlags(result, OperandWidth(instruction.operands[0]));
                        return;
                    }

                    case Mnemonic::And:
                    {
                        llvm::Value* result = m_builder.CreateAnd(ReadOperand(instruction.operands[0]),
                                                                  ReadOperand(instruction.operands[1]));
                        WriteOperand(instruction.operands[0], result);
                        SetResultFlags(result, OperandWidth(instruction.operands[0]));
                        return;
                    }

                    case Mnemonic::Not:
                        WriteOperand(instruction.operands[0],
                                     m_builder.CreateNot(ReadOperand(instruction.operands[0])));
                        return;

                    case Mnemonic::Neg:
                    {
                        llvm::Value* result = m_builder.CreateNeg(ReadOperand(instruction.operands[0]));
                        WriteOperand(instruction.operands[0], result);
                        SetResultFlags(result, OperandWidth(instruction.operands[0]));
                        return;
                    }

                    case Mnemonic::Shl:
                    {
                        const unsigned width = OperandWidth(instruction.operands[0]);
                        llvm::Value* result = m_builder.CreateShl(ReadOperand(instruction.operands[0]),
                                                                  ShiftCount(instruction.operands[1], width));
                        WriteOperand(instruction.operands[0], result);
                        SetResultFlags(result, width);
                        return;
                    }

                    case Mnemonic::Shr:
                    {
                        const unsigned width = OperandWidth(instruction.operands[0]);
                        llvm::Value* result = m_builder.CreateLShr(ReadOperand(instruction.operands[0]),
                                                                   ShiftCount(instruction.operands[1], width));
                        WriteOperand(instruction.operands[0], result);
                        SetResultFlags(result, width);
                        return;
                    }

                    case Mnemonic::Sar:
                    {
                        const unsigned width = OperandWidth(instruction.operands[0]);
                        llvm::Value* result = m_builder.CreateAShr(ReadOperand(instruction.operands[0]),
                                                                   ShiftCount(instruction.operands[1], width));
                        WriteOperand(instruction.operands[0], result);
                        SetResultFlags(result, width);
                        return;
                    }

                    case Mnemonic::Rol:
                    case Mnemonic::Ror:
                    {
                        const unsigned width = OperandWidth(instruction.operands[0]);
                        llvm::Type* rotateType = IntegerType(width);
                        llvm::Value* value = m_builder.CreateTrunc(ReadOperand(instruction.operands[0]), rotateType);
                        llvm::Value* count =
                            m_builder.CreateTrunc(ShiftCount(instruction.operands[1], width), rotateType);
                        const llvm::Intrinsic::ID intrinsic =
                            (instruction.mnemonic == Mnemonic::Rol) ? llvm::Intrinsic::fshl : llvm::Intrinsic::fshr;
                        llvm::Value* rotated =
                            m_builder.CreateIntrinsic(intrinsic, {rotateType}, {value, value, count});
                        WriteOperand(instruction.operands[0], m_builder.CreateZExt(rotated, m_int64));
                        m_carryConditionsSupported = false;
                        return;
                    }

                    case Mnemonic::Inc:
                    {
                        llvm::Value* result = m_builder.CreateAdd(ReadOperand(instruction.operands[0]), Constant64(1));
                        WriteOperand(instruction.operands[0], result);
                        SetResultFlags(result, OperandWidth(instruction.operands[0]));
                        return;
                    }

                    case Mnemonic::Dec:
                    {
                        llvm::Value* result = m_builder.CreateSub(ReadOperand(instruction.operands[0]), Constant64(1));
                        WriteOperand(instruction.operands[0], result);
                        SetResultFlags(result, OperandWidth(instruction.operands[0]));
                        return;
                    }

                    case Mnemonic::Cmp:
                        SetComparisonFlags(ReadOperand(instruction.operands[0]), ReadOperand(instruction.operands[1]),
                                           OperandWidth(instruction.operands[0]));
                        return;

                    case Mnemonic::Test:
                    {
                        llvm::Value* result = m_builder.CreateAnd(ReadOperand(instruction.operands[0]),
                                                                  ReadOperand(instruction.operands[1]));
                        SetResultFlags(result, OperandWidth(instruction.operands[0]));
                        return;
                    }

                    case Mnemonic::SetConditional:
                        WriteOperand(instruction.operands[0],
                                     m_builder.CreateZExt(EmitCondition(instruction.condition), m_int64));
                        return;

                    case Mnemonic::ConditionalJump:
                        m_builder.CreateCondBr(EmitCondition(instruction.condition), BlockAt(block.successors[0]),
                                               BlockAt(block.successors[1]));
                        return;

                    case Mnemonic::Jmp:
                        m_builder.CreateBr(BlockAt(block.successors.front()));
                        return;

                    case Mnemonic::Push:
                    {
                        llvm::Value* pointer =
                            m_builder.CreateSub(ReadRegister(GpRegister::Rsp, 64, false), Constant64(8));
                        WriteRegister(GpRegister::Rsp, 64, false, pointer);
                        StoreMemory(pointer, 64, ReadOperand(instruction.operands[0]), true);
                        return;
                    }

                    case Mnemonic::Pop:
                    {
                        llvm::Value* pointer = ReadRegister(GpRegister::Rsp, 64, false);
                        WriteOperand(instruction.operands[0], LoadMemory(pointer, 64, true));
                        WriteRegister(GpRegister::Rsp, 64, false, m_builder.CreateAdd(pointer, Constant64(8)));
                        return;
                    }

                    case Mnemonic::Ret:
                        m_builder.CreateRet(ReadRegister(GpRegister::Rax, 64, false));
                        return;

                    case Mnemonic::Nop:
                        return;

                    default:
                        throw std::runtime_error("lift: unsupported instruction at 0x" + Hex(instruction.address) +
                                                 " (" + instruction.text + ")");
                }
            }

            void EmitBlock(const BasicBlock& block)
            {
                m_builder.SetInsertPoint(BlockAt(block.startAddress));
                m_flagLeft = nullptr;
                m_flagRight = nullptr;
                m_flagWidth = 0;
                m_carryConditionsSupported = false;

                for (const DecodedInstruction& instruction : block.instructions)
                    EmitInstruction(instruction, block);

                const DecodedInstruction& last = block.instructions.back();
                const bool terminated = last.mnemonic == Mnemonic::Ret || last.mnemonic == Mnemonic::Jmp ||
                                        last.mnemonic == Mnemonic::ConditionalJump;
                if (!terminated) m_builder.CreateBr(BlockAt(block.successors.front()));
            }

            llvm::LLVMContext& m_context;
            llvm::Module& m_module;
            const ControlFlowGraph& m_graph;
            llvm::IRBuilder<> m_builder;
            llvm::Type* m_int64;
            llvm::Function* m_function = nullptr;
            std::map<GpRegister, llvm::AllocaInst*> m_slots;
            std::map<std::uint64_t, llvm::BasicBlock*> m_blocks;
            llvm::Value* m_stackStart = nullptr;
            llvm::Value* m_stackEnd = nullptr;
            llvm::Value* m_flagLeft = nullptr;
            llvm::Value* m_flagRight = nullptr;
            unsigned m_flagWidth = 0;
            bool m_flagsFromComparison = false;
            bool m_carryConditionsSupported = false;
        };
    }

    std::unique_ptr<Ir::IIrModule> X86ToLlvmLifter::Lift(const ControlFlowGraph& graph) const
    {
        auto context = std::make_unique<llvm::LLVMContext>();
        auto module = std::make_unique<llvm::Module>("lifter", *context);
        module->setTargetTriple("x86_64-pc-windows-msvc");

        FunctionLifter lifter(*context, *module, graph);
        lifter.Lift();

        return std::make_unique<Ir::OwnedLlvmModule>(std::move(context), std::move(module), graph.name);
    }
}
