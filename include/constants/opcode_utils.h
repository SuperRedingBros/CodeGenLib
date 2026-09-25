#ifndef CODE_GEN_TESTS_OPCODE_UTILS_H
#define CODE_GEN_TESTS_OPCODE_UTILS_H

#include "Registers.h"

#define OP_ARG_ADD 0b000
#define OP_ARG_SUB 0b101
#define OP_ARG_CMP 0b111
#define OP_ARG_JUMP 0b100
#define OP_ARG_CALL 0b010

enum class Mod_RM_OPArg {
    ADD = 0b000,
    SUB = 0b101,
    CMP = 0b111,
    JMP = 0b100,
    CALL = 0b010,
};

enum class Mod_RM_AddressMode {
    INDIRECT_NO_DISPLACEMENT,
    INDIRECT_8Bit_DISPLACEMENT,
    INDIRECT_32Bit_DISPLACEMENT,
    DIRECT,
    RIP_32Bit_DISPLACEMENT,
    RBP_8Bit_DISPLACEMENT,
    RBP_32Bit_DISPLACEMENT,
};


constexpr uint8_t makeMod_RM_Byte(const Mod_RM_AddressMode mode, const uint8_t regBits, const uint8_t rmBits) {
    uint8_t modBits = 0b00;
    switch (mode) {
        case Mod_RM_AddressMode::INDIRECT_NO_DISPLACEMENT:
            modBits = 0b00;
            break;
        case Mod_RM_AddressMode::INDIRECT_8Bit_DISPLACEMENT:
            modBits = 0b01;
            break;
        case Mod_RM_AddressMode::INDIRECT_32Bit_DISPLACEMENT:
            modBits = 0b10;
            break;
        case Mod_RM_AddressMode::DIRECT:
            modBits = 0b11;
            break;
        case Mod_RM_AddressMode::RIP_32Bit_DISPLACEMENT:
            modBits = 0b00;
            break;
        case Mod_RM_AddressMode::RBP_8Bit_DISPLACEMENT:
            modBits = 0b01;
            break;
        case Mod_RM_AddressMode::RBP_32Bit_DISPLACEMENT:
            modBits = 0b10;
            break;
    }
    return (modBits & 0b11) << 6 | (regBits & 0b111) << 3 | (rmBits & 0b111);
}


constexpr uint8_t makeRexPrefix(
    const bool is64Bit = true,
    const bool modRMReg = false,
    const bool SIBIndex = false,
    const bool modRM_RM = false) {
    return 0b01000000
        | (is64Bit ? 1 << 3 : 0)
        | (modRMReg ? 1 << 2 : 0)
        | (SIBIndex ? 1 << 1 : 0)
        | (modRM_RM ? 1 << 0 : 0);
}

enum class ConditionModes : uint8_t {
    OVERFLOW, NOT_OVERFLOW, CARRY, NOT_CARRY,
    ZERO, NOT_ZERO, BELOW_OR_EQUAL, ABOVE,
    SIGN, NOT_SIGN, PARITY_EVEN, PARITY_ODD,
    LESS_THAN, GREATER_OR_EQUAL, LESS_OR_EQUAL, GREATER
};

constexpr uint8_t ConditionToJumpOpcode(ConditionModes mode) {
    return 0x70 | static_cast<uint8_t>(mode);
}

#endif //CODE_GEN_TESTS_OPCODE_UTILS_H
