#include "../include/CodeBuffer.h"

#include <sys/stat.h>

#include "../include/Async.h"
#include "../include/Assembler.h"
#include "../include/LinkingInfo.h"

CodeLabel * CodeBuffer::ReserveLabel(const std::string &label) const {
    return owningAssembler->ReserveLabel(currentFunction->name + "$" + label);
}

void CodeBuffer::async_pause() {
    callLinkedFunction(owningAssembler->GetLabel("Linked_async_pause"));
}

void CodeBuffer::async_raise() {
    callLinkedFunction(owningAssembler->GetLabel("Linked_async_raise"));
}

/**
 * Puts a coroutine to sleep for RDX milliseconds
 */
void CodeBuffer::async_sleep() {
    callLinkedFunction(owningAssembler->GetLabel("Linked_async_sleep"));
}

CodeLabel * CodeBuffer::jumpToLabel() {
    CodeLabel* l = ReserveLabel("label_" + std::to_string(size()));
    jumpToLabel(l);
    return l;
}

void CodeBuffer::jumpToLabel(const CodeLabel *label)  {
    owningAssembler->addRelocation(new Relocation(currentFunction, size()+1, 4, label));
    putBytes({
        0xE9,
        0x07,
        0x00,
        0x00,
        0x00
    });
}

void CodeBuffer::block(const std::string &name,
    const std::function<void(const CodeLabel *, const CodeLabel *, CodeBuffer*)> &func) {
    const CodeLabel* start = addLabel(name + "_start");
    CodeLabel* end = ReserveLabel(name + "_end");
    func(start, end, this);
    markLabel(end);
}

const CodeLabel * CodeBuffer::jumpToLabelCond(const ConditionModes condition) {
    const CodeLabel* l = ReserveLabel("label_" + std::to_string(size()));
    jumpToLabelCond(l, condition);
    return l;
}

void CodeBuffer::callLinkedFunction(const CodeLabel* function) {
    owningAssembler->addRelocation(new Relocation(currentFunction, size() + 2, 4,
        owningAssembler->GetSegment("LinkingInfo"), function));
    putBytes({0xFF,
        makeMod_RM_Byte(Mod_RM_AddressMode::INDIRECT_NO_DISPLACEMENT, 0b010, 0b101),
        0x00, 0x00, 0x00, 0x00
    });
}

void CodeBuffer::callLinkedFunction(const LinkingInfo* linking_info, const std::string& function_name) {
    owningAssembler->addRelocation(new Relocation(currentFunction, size() + 2, 4,
        owningAssembler->GetSegment("LinkingInfo"), linking_info->GetLinkedFunction(owningAssembler, function_name)));
    putBytes({0xFF,
        makeMod_RM_Byte(Mod_RM_AddressMode::INDIRECT_NO_DISPLACEMENT, 0b010, 0b101),
        0x00, 0x00, 0x00, 0x00
    });
}


void CodeBuffer::jumpToLabelCond(const CodeLabel *label, const ConditionModes condition) {
    owningAssembler->addRelocation(new Relocation(currentFunction, size()+1, 1, label));
    putBytes({
        ConditionToJumpOpcode(condition),
        0x00
    });
}

void CodeBuffer::callFunction(const FunctionHandle *target)  {
    owningAssembler->addRelocation(new Relocation(currentFunction, size() + 1, 4, target));
    putBytes({
        0xE8,
        0x00,
        0x00,
        0x00,
        0x00
    });
}

void CodeBuffer::movReg(const X64SSERegister dstReg, const Segment* segment, const X64SSERegisterPrecision precision)  {
    owningAssembler->addRelocation(new Relocation(currentFunction, size()+4, 4, segment));
    switch (precision) {
        case X64SSERegisterPrecision::SCALER_SINGLE_PRECISION:
            putBytes(0xf3);
            break;
        case X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION:
            putBytes(0xf2);
            break;
    }
    putBytes({
        0x0f, 0x10, makeMod_RM_Byte(Mod_RM_AddressMode::INDIRECT_NO_DISPLACEMENT, static_cast<uint8_t>(dstReg), 0b101), //0b00 | 0b100 | 0b101,
            0x00, 0x00, 0x00, 0x00
    });
}

void CodeBuffer::movReg(const X64SSERegister dstReg, const X64MemoryAddress &address, const X64SSERegisterPrecision precision) {
    if (precision == X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION)
        putBytes(0x66);
    if (dstReg >= X64SSERegister::XMM8) setREXExtendedReg();
    if (address.base >= X64Register::R8) setREXExtendedRM();
    if (address.index >= X64Register::R8) setREXExtendedSIB();
    putBytes({
        0x0f, 0x28
    });
    putBytes(address.makeMemoryAddressingBytes(dstReg));
}

void CodeBuffer::movReg(const X64SSERegister dstReg, const X64SSERegister srcReg, const X64SSERegisterPrecision precision)  {
    if (precision == X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION)
        putBytes(0x66);
    putBytes({
        0x0f, 0x28, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, static_cast<uint8_t>(dstReg), static_cast<uint8_t>(srcReg))
    });
}

void CodeBuffer::mulReg(const X64Register low_result, const X64Register high_result, const X64Register src1, const X64Register src2,
                        const bool is_signed) {
    if (src1 != X64Register::RAX) {
        exchangeRegs(X64Register::RAX, src1);
    }
    if (high_result != X64Register::RDX) {
        exchangeRegs(X64Register::RDX, high_result);
    }

    setREX64();
    if (src2 >= X64Register::R8) setREXExtendedRM();
    putBytes({
        0xf7, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, 4 + is_signed, static_cast<uint8_t>(src2))
    });
    if (low_result != X64Register::RAX) {
        movReg(low_result, X64Register::RAX);
        if (src1 != X64Register::RAX)
            exchangeRegs(X64Register::RAX, src1);
    }
    if (high_result != X64Register::RDX) {
        exchangeRegs(high_result, X64Register::RDX);
    }
}

void CodeBuffer::divReg(const X64Register quotient, const X64Register remainder, const X64Register numerator,
                        const X64Register denominator, const bool is_signed) {
    if (numerator != X64Register::RAX) {
        exchangeRegs(X64Register::RAX, numerator);
    }
    if (remainder != X64Register::RDX) {
        exchangeRegs(X64Register::RDX, remainder);
    }

    if (is_signed) {
        setREX64(); //Width extension of rax -> rdx:rax
        putOpcode(0x99);
    }
    else
        clearReg(X64Register::RDX);

    setREX64();
    if (denominator >= X64Register::R8)
        setREXExtendedRM();
    putBytes({
        0xf7, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, 6 + is_signed, static_cast<uint8_t>(denominator))
    });
    if (quotient != X64Register::RAX) {
        movReg(quotient, X64Register::RAX);
        if (numerator != X64Register::RAX)
            exchangeRegs(X64Register::RAX, numerator);
    }
    if (remainder != X64Register::RDX) {
        exchangeRegs(remainder, X64Register::RDX);
    }
}

void CodeBuffer::leaReg(X64Register dstReg, const Segment *address)  {
    owningAssembler->addRelocation(new Relocation(currentFunction, size()+3, 4, address));
    setREX64();
    if (dstReg >= X64Register::R8) setREXExtendedReg();
    putBytes({
        0x8d, makeMod_RM_Byte(Mod_RM_AddressMode::INDIRECT_NO_DISPLACEMENT, static_cast<uint8_t>(dstReg), 0b101), //0b00 | 0b100 | 0b101,
            0x00, 0x00, 0x00, 0x00
    });
}

void CodeBuffer::leaReg(const X64Register dstReg, const CodeLabel *label) {
    owningAssembler->addRelocation(new Relocation(currentFunction, size()+3, 4, label));
    putBytes({
        makeRexPrefix(true), 0x8d, makeMod_RM_Byte(Mod_RM_AddressMode::RIP_32Bit_DISPLACEMENT, static_cast<uint8_t>(dstReg), 0b101),
        0xFF, 0xFE, 0xFD, 0xFC
    });
}

void CodeBuffer::movReg_mem(const X64Register dstReg, const Segment* segment) {
    owningAssembler->addRelocation(new Relocation(currentFunction, size()+3, 4, segment));
    putBytes({
        makeRexPrefix(true), 0x8b, makeMod_RM_Byte(Mod_RM_AddressMode::INDIRECT_NO_DISPLACEMENT, static_cast<uint8_t>(dstReg), 0b101), //0b00 | 0b100 | 0b101,
            0x00, 0x00, 0x00, 0x00
    });
}

void CodeBuffer::exchangeRegs(const X64Register dstReg, const X64Register srcReg) {
    setREX64();
    if (dstReg >= X64Register::R8) setREXExtendedReg();
    if (srcReg >= X64Register::R8) setREXExtendedRM();
    putBytes({
        0x87, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, static_cast<uint8_t>(dstReg), static_cast<uint8_t>(srcReg))
    });
}

void CodeBuffer::addReg(const X64SSERegister dstReg, const X64SSERegister srcReg, const X64SSERegisterPrecision precision) {
    switch (precision) {
        case X64SSERegisterPrecision::SCALER_SINGLE_PRECISION:
            putBytes(0xf3);
            break;
        case X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION:
            putBytes(0xf2);
    }
    putBytes({0x0f, 0x58, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, static_cast<uint8_t>(dstReg), static_cast<uint8_t>(srcReg))});
}

void CodeBuffer::subReg(const X64SSERegister dstReg, const X64SSERegister srcReg, const X64SSERegisterPrecision precision) {
    switch (precision) {
        case X64SSERegisterPrecision::SCALER_SINGLE_PRECISION:
            putBytes(0xf3);
            break;
        case X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION:
            putBytes(0xf2);
    }
    putBytes({0x0f, 0x5c, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, static_cast<uint8_t>(dstReg), static_cast<uint8_t>(srcReg))});
}

void CodeBuffer::mulReg(const X64SSERegister dstReg, const X64SSERegister srcReg, const X64SSERegisterPrecision precision) {
    switch (precision) {
        case X64SSERegisterPrecision::SCALER_SINGLE_PRECISION:
            putBytes(0xf3);
            break;
        case X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION:
            putBytes(0xf2);
    }
    putBytes({0x0f, 0x59, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, static_cast<uint8_t>(dstReg), static_cast<uint8_t>(srcReg))});
}

void CodeBuffer::divReg(const X64SSERegister dstReg, const X64SSERegister srcReg, const X64SSERegisterPrecision precision) {
    switch (precision) {
        case X64SSERegisterPrecision::SCALER_SINGLE_PRECISION:
            putBytes(0xf3);
            break;
        case X64SSERegisterPrecision::SCALER_DOUBLE_PRECISION:
            putBytes(0xf2);
    }
    putBytes({0x0f, 0x5e, makeMod_RM_Byte(Mod_RM_AddressMode::DIRECT, static_cast<uint8_t>(dstReg), static_cast<uint8_t>(srcReg))});
}