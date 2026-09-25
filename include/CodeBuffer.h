#ifndef CODE_GEN_TESTS_CODEBUFFER_H
#define CODE_GEN_TESTS_CODEBUFFER_H
#include <cstdint>
#include <functional>
#include <ranges>
#include <string>
#include <vector>

#include "constants/opcode_utils.h"
#include "constants/Registers.h"
#include "constants/RegSize.h"

#include "FunctionHandle.h"
#include "MemoryAddress.h"
#include "Relocation.h"

class LinkingInfo;

class CodeBuffer {
    Assembler* owningAssembler;
    FunctionHandle* currentFunction;
    std::vector<X64Register> preservedRegisters{};

    TBuffer buffer;
    uint32_t rex: 1 = 0;
public:
    size_t reservedStack = 0;
    uint8_t prologSize = 0;

    CodeBuffer(Assembler* assembler, FunctionHandle* currentFunction) :
        owningAssembler(assembler), currentFunction(currentFunction) {
    }

    // -----------------------------------------------------------------------------------------------
    // Buffer interaction
    // -----------------------------------------------------------------------------------------------

    [[nodiscard]] size_t size() const {
        return buffer.size();
    }

    [[nodiscard]] const TBuffer* bufferRef() const {
        return &buffer;
    }

    void putBytes(const uint8_t byte) {
        buffer.pushByte(byte);
        rex = 0;
    }

    void putBytes(const std::initializer_list<uint8_t> bytes) {
        buffer.pushBytes(bytes);
        rex = 0;
    }

    void setBytes(const size_t position, const std::initializer_list<uint8_t> bytes) {
        buffer.setBytes(position, bytes);
        rex = 0;
    }

    void putBytes(const std::vector<uint8_t> &bytes) {
        buffer.pushBytes(bytes);
        rex = 0;
    }

    // -----------------------------------------------------------------------------------------------
    // Opcode utils
    // -----------------------------------------------------------------------------------------------

    void setREX64() {
        if (rex == 1) {
            buffer.top() |= (1 << 3);
            return;
        }
        rex = 1;
        buffer.pushByte(makeRexPrefix(true));
    }

    void setREXExtendedReg() {
        if (rex == 1) {
            buffer.top() |= (1 << 2);
            return;
        }
        rex = 1;
        buffer.pushByte(makeRexPrefix(false, true));
    }

    void setREXExtendedRM() {
        if (rex == 1) {
            buffer.top() |= (1 << 0);
            return;
        }
        rex = 1;
        buffer.pushByte(makeRexPrefix(false, false, false, true));
    }

    void setREXExtendedSIB() {
        if (rex == 1) {
            buffer.top() |= (1 << 1);
            return;
        }
        rex = 1;
        buffer.pushByte(makeRexPrefix(false, false, true, false));
    }

    void setRegSize(const RegSize regSize) {
        if (regSize == RegSize::X16) {
            putBytes({0x66});
        } else if (regSize == RegSize::X64) {
            setREX64();
        }
    }

    void putOpcode(const uint8_t opcode) {
        putBytes(opcode);
    }

    void putOpcode(const uint8_t opcode, const X64Register target_reg) {
        putBytes(opcode | (static_cast<uint8_t>(target_reg) & 0b111));
    }

    // -----------------------------------------------------------------------------------------------
    // Push/Pop
    // -----------------------------------------------------------------------------------------------

    void pushReg(const X64Register reg) {
        if (reg >= X64Register::R8) setREXExtendedRM();
        putOpcode(0x50, reg);
    }

    void popReg(const X64Register reg) {
        if (reg >= X64Register::R8) setREXExtendedRM();
        putOpcode(0x58, reg);
    }

    // -----------------------------------------------------------------------------------------------
    // Misc
    // -----------------------------------------------------------------------------------------------

    void ret() {
        // Return from near procedure - On modern far-calls are mostly unnecessary
        putBytes(0xC3);
    }

    void negReg(X64Register reg) {
        setREX64();
        if (reg >= X64Register::R8) setREXExtendedRM();
        putBytes({
            0xf7, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, 3, static_cast<uint8_t>(reg))
        });
    }

    // -----------------------------------------------------------------------------------------------
    // Label utils
    // -----------------------------------------------------------------------------------------------

    [[nodiscard]] const CodeLabel* addLabel(const std::string& label) const {
        CodeLabel* l = ReserveLabel(label);
        l->offsetInFunc = size();
        return l;
    }

    [[nodiscard]] CodeLabel* ReserveLabel(const std::string& label) const;

    void markLabel(CodeLabel* label) const {
        label->offsetInFunc = size();
    }

    // -----------------------------------------------------------------------------------------------
    // Async
    // -----------------------------------------------------------------------------------------------

    void async_pause();

    void async_raise();

    void async_sleep();

    // -----------------------------------------------------------------------------------------------
    // Jumps
    // -----------------------------------------------------------------------------------------------

    CodeLabel* jumpToLabel();

    void jumpToLabel(const CodeLabel *label);

    const CodeLabel* jumpToLabelCond(ConditionModes condition);

    void jumpToLabelCond(const CodeLabel *label, ConditionModes condition);

    void jumpToReg(const X64Register reg) {
        putBytes({
            0xFF,
            makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, OP_ARG_JUMP, static_cast<uint8_t>(reg))
        });
    }

    // -----------------------------------------------------------------------------------------------
    // Calls
    // -----------------------------------------------------------------------------------------------

    void callLinkedFunction(const CodeLabel *function);

    void callLinkedFunction(const LinkingInfo *linking_info, const std::string &function_name);

    void callReg(const X64Register reg) {
        putBytes({0xFF,
            makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, OP_ARG_CALL, static_cast<uint8_t>(reg))});
    }

    void call(const X64MemoryAddress address) {
        putBytes({0xFF, makeMod_RM_Byte(Mod_RM_AddressMode::INDIRECT_NO_DISPLACEMENT, 0b010, static_cast<uint8_t>(address.base))});
    }

    void callFunction(const FunctionHandle *target);

    // -----------------------------------------------------------------------------------------------
    // Floating point arithmatic
    // -----------------------------------------------------------------------------------------------

    void addReg(X64SSERegister dstReg, X64SSERegister srcReg, X64SSERegisterPrecision precision = X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION);

    void subReg(X64SSERegister dstReg, X64SSERegister srcReg, X64SSERegisterPrecision precision = X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION);

    void mulReg(X64SSERegister dstReg, X64SSERegister srcReg, X64SSERegisterPrecision precision = X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION);

    void divReg(X64SSERegister dstReg, X64SSERegister srcReg, X64SSERegisterPrecision precision = X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION);

    // -----------------------------------------------------------------------------------------------
    // Integer arithmatic
    // -----------------------------------------------------------------------------------------------

    void incReg(const X64Register regDst) {
        setREX64();
        putBytes({
            0xFF, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, 0, static_cast<uint8_t>(regDst))
        });
    }

    void decReg(const X64Register regDst) {
        setREX64();
        putBytes({
            0xFF, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, 1, static_cast<uint8_t>(regDst))
        });
    }

    void cmpReg(const X64Register regDst, const X64Register regSrc) {
        setREX64();
        if (regDst >= X64Register::R8) setREXExtendedRM();
        if (regSrc >= X64Register::R8) setREXExtendedReg();
        putBytes({
            0x39, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, static_cast<uint8_t>(regSrc), static_cast<uint8_t>(regDst))
        });
    }
    void cmpReg(const X64Register regDst, const uint64_t value) {
        if (value <= 65535 && value > 255) {
            putBytes(0x66);
        }
        setREX64();
        if (regDst >= X64Register::R8) setREXExtendedRM();
        if (value <= 255)
            putBytes({
                0x83,
                makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, OP_ARG_CMP, static_cast<uint8_t>(regDst)),
                static_cast<uint8_t>(value)
            });
        else if (value <= 65535)
            putBytes({0x81, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, OP_ARG_CMP, static_cast<uint8_t>(regDst)),
                static_cast<uint8_t>(value & 0xff), static_cast<uint8_t>(value >> 8 & 0xff),});
    }

    void addReg(const X64Register regDst, const X64Register regSrc) {
        setREX64();
        if (regDst >= X64Register::R8) setREXExtendedRM();
        if (regSrc >= X64Register::R8) setREXExtendedReg();
        putBytes({
            0x01, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, static_cast<uint8_t>(regSrc), static_cast<uint8_t>(regDst))
        });
    }
    void addReg(const X64Register regDst, const uint64_t value) {
        if (value <= 65535 && value > 255) {
            putBytes(0x66);
        }
        setREX64();
        if (regDst >= X64Register::R8) setREXExtendedRM();
        if (value == 1)
            incReg(regDst);
        else if (value == -1)
            decReg(regDst);
        else if (value <= 255)
            putBytes({
                0x83,
                makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, OP_ARG_ADD, static_cast<uint8_t>(regDst)),
                static_cast<uint8_t>(value)
            });
        else if (value <= 65535)
            putBytes({0x81, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, OP_ARG_ADD, static_cast<uint8_t>(regDst)),
                static_cast<uint8_t>(value & 0xff), static_cast<uint8_t>(value >> 8 & 0xff),});
    }

    void subReg(const X64Register regDst, const X64Register regSrc) {
        setREX64();
        if (regDst >= X64Register::R8) setREXExtendedRM();
        if (regSrc >= X64Register::R8) setREXExtendedReg();
        putBytes({
            0x29, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, static_cast<uint8_t>(regSrc), static_cast<uint8_t>(regDst))
        });
    }
    void subReg(const X64Register regDst, const uint64_t value) {
        if (value <= 65535 && value > 255) {
            putBytes(0x66);
        }
        setREX64();
        if (regDst >= X64Register::R8) setREXExtendedRM();
        if (value == 1)
            decReg(regDst);
        else if (value == -1)
            incReg(regDst);
        else if (value <= 255)
            putBytes({
                0x83,
                makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, OP_ARG_SUB, static_cast<uint8_t>(regDst)),
                static_cast<uint8_t>(value)
            });
        else if (value <= 65535)
            putBytes({0x81, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, OP_ARG_SUB, static_cast<uint8_t>(regDst)),
                static_cast<uint8_t>(value & 0xff), static_cast<uint8_t>(value >> 8 & 0xff),});
    }


    void mulReg(X64Register low_result, X64Register high_result, X64Register src1, X64Register src2, bool is_signed = true);
    void divReg(X64Register quotient, X64Register remainder, X64Register numerator, X64Register denominator, bool is_signed = true);

    // -----------------------------------------------------------------------------------------------
    // Address loads
    // -----------------------------------------------------------------------------------------------

    void loadRIPToReg(const X64Register dstReg, const int32_t offset = 0) {
        putBytes({
            makeRexPrefix(true), 0x8d, makeMod_RM_Byte(Mod_RM_AddressMode::INDIRECT_NO_DISPLACEMENT, static_cast<uint8_t>(dstReg), 0b101), //0b00 | 0b100 | 0b101,
            static_cast<uint8_t>(offset & 0xff),
            static_cast<uint8_t>(offset >> 8 & 0xff),
            static_cast<uint8_t>(offset >> 16 & 0xff),
            static_cast<uint8_t>(offset >> 24 & 0xff),
        });
    }

    void leaReg(X64Register dstReg, const Segment *address);

    void leaReg(X64Register dstReg, const CodeLabel *label);

    // -----------------------------------------------------------------------------------------------
    // Register moves
    // -----------------------------------------------------------------------------------------------

    void clearReg(const X64Register dstReg) {
        setREX64();
        if (dstReg >= X64Register::R8) {
            setREXExtendedRM();
            setREXExtendedReg();
        }
        putBytes({
            0x31,
            makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, static_cast<uint8_t>(dstReg), static_cast<uint8_t>(dstReg)),
        });
    }

    void movReg(X64SSERegister dstReg, X64SSERegister srcReg, X64SSERegisterPrecision precision = X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION);

    void moveRegToRegX64(const X64Register regDst, const X64Register regSrc) {
        setREX64();
        if (regSrc >= X64Register::R8) setREXExtendedReg();
        if (regDst >= X64Register::R8) setREXExtendedRM();
        putOpcode(0x89);
        putBytes({
            makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, static_cast<uint8_t>(regSrc), static_cast<uint8_t>(regDst))
        });
    }

    void moveValToRegX64(const X64Register regDst, const uint64_t value) {
        setREX64();
        if (regDst >= X64Register::R8) setREXExtendedRM();
        putOpcode(0xb8, regDst);
        putBytes({
            static_cast<unsigned char>(value & 0xff), \
            static_cast<unsigned char>((value >> 8) & 0xff), \
            static_cast<unsigned char>((value >> 16) & 0xff), \
            static_cast<unsigned char>((value >> 24) & 0xff), \
            static_cast<unsigned char>((value >> 32) & 0xff), \
            static_cast<unsigned char>((value >> 40) & 0xff), \
            static_cast<unsigned char>((value >> 48) & 0xff), \
            static_cast<unsigned char>((value >> 56) & 0xff)
        });
    }

    void moveValToRegX32(const X64Register regDst, const uint32_t value) {
        if (value == 0) {
            clearReg(regDst);
            return;
        }
        if (regDst >= X64Register::R8) setREXExtendedRM();
        putOpcode(0xb8, regDst);
        putBytes({
            static_cast<unsigned char>(value & 0xff),
            static_cast<unsigned char>((value >> 8) & 0xff),
            static_cast<unsigned char>((value >> 16) & 0xff),
            static_cast<unsigned char>((value >> 24) & 0xff),
        });
    }

    void moveValToRegX16(const X64Register regDst, const uint16_t value) {
        if (value == 0) {
            clearReg(regDst);
            return;
        }
        putBytes(0x66);
        if (regDst >= X64Register::R8) {
            setREXExtendedRM();
        }
        putOpcode(0xb8, regDst);
        putBytes({
            static_cast<unsigned char>(value & 0xff),
            static_cast<unsigned char>((value >> 8) & 0xff),
        });
    }

    void moveValToReg(const X64Register regDst, const uint64_t value) {
        if (value < static_cast<uint16_t>(-1)) {
            clearReg(regDst);
            moveValToRegX16(regDst, value);
            return;
        }
        if (value < static_cast<uint32_t>(-1)) {
            moveValToRegX32(regDst, value);
            return;
        }
        moveValToRegX64(regDst, value);
    }

    void movReg(const X64Register dstReg, const X64Register srcReg) {
        moveRegToRegX64(dstReg, srcReg);
    }

    void movReg(const X64SSERegister dstReg, const X64Register srcReg, const RegSize regSize = RegSize::X64) {
        putBytes(0x66);
        if (regSize == RegSize::X64) setREX64();
        if (srcReg >= X64Register::R8) setREXExtendedReg();
        if (dstReg >= X64SSERegister::XMM8) setREXExtendedRM();
        putBytes({
            0x0f, 0x6e, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, static_cast<uint8_t>(dstReg), static_cast<uint8_t>(srcReg)),
        });
    }

    void movReg(const X64Register dstReg, const X64SSERegister srcReg, const RegSize regSize = RegSize::X64) {
        putBytes(0x66);
        if (regSize == RegSize::X64) setREX64();
        if (srcReg >= X64SSERegister::XMM8) setREXExtendedRM();
        if (dstReg >= X64Register::R8) setREXExtendedReg();
        putBytes({
            0x0f, 0x7e, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, static_cast<uint8_t>(srcReg), static_cast<uint8_t>(dstReg)),
        });
    }

    void movReg(const X64Register dstReg, const X64MemoryAddress address, const RegSize regSize = RegSize::X8) {
        setRegSize(regSize);
        if (dstReg >= X64Register::R8) {
            setREXExtendedReg();
        }
        if (address.base >= X64Register::R8) {
            setREXExtendedRM();
        }
        if (address.index >= X64Register::R8) {
            setREXExtendedSIB();
        }
        const uint8_t opcode = regSize == RegSize::X8 ? 0x8A : 0x8B;
        putOpcode(opcode);
        putBytes(address.makeMemoryAddressingBytes(dstReg));
    }

    void movReg(const X64MemoryAddress address, const X64Register dstReg, const RegSize regSize = RegSize::X8) {
        setRegSize(regSize);
        if (dstReg >= X64Register::R8) {
            setREXExtendedReg();
        }
        if (address.base >= X64Register::R8) {
            setREXExtendedRM();
        }
        if (address.index >= X64Register::R8) {
            setREXExtendedSIB();
        }
        const uint8_t opcode = regSize == RegSize::X8 ? 0x88 : 0x89;
        putOpcode(opcode);
        putBytes(address.makeMemoryAddressingBytes(dstReg));
    }

    void movReg(const X64Register dstReg, const uint64_t value) {
        moveValToReg(dstReg, value);
    }

    void movReg(X64SSERegister dstReg, const Segment *segment, X64SSERegisterPrecision precision = X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION);
    void movReg(X64SSERegister dstReg, const X64MemoryAddress& address, X64SSERegisterPrecision precision = X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION);

    void movReg_mem(X64Register dstReg, const Segment *segment);

    void exchangeRegs(X64Register dstReg, X64Register srcReg);

    // -----------------------------------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------------------------------

    void prolog(const uint8_t stackSize, const std::initializer_list<X64Register> regsToPreserve) {
        reservedStack = stackSize;
        preservedRegisters.clear();
        preservedRegisters.reserve(regsToPreserve.size());
        preservedRegisters.insert(preservedRegisters.end(), regsToPreserve.begin(), regsToPreserve.end());

        pushReg(X64Register::RBP);
        for (const auto regToPreserve: preservedRegisters) {
            pushReg(regToPreserve);
        }
        subReg(X64Register::RSP, reservedStack);
        moveRegToRegX64(X64Register::RBP, X64Register::RSP);
        prologSize = static_cast<uint8_t>(size());
    }
    void epilog() {
        addReg(X64Register::RSP, reservedStack);
        for (const auto preservedRegister : std::views::reverse(preservedRegisters)) {
            popReg(preservedRegister);
        }
        popReg(X64Register::RBP);
    }

    void block(const std::string& name, const std::function<void(const CodeLabel*, const CodeLabel*, CodeBuffer*)>& func);

    void block(const std::function<void(const CodeLabel*, const CodeLabel*, CodeBuffer*)>& func) {
        const std::string name = "label_" + std::to_string(size());
        block(name, func);
    }
};


#endif //CODE_GEN_TESTS_CODEBUFFER_H
