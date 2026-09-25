#ifndef CODE_GEN_TESTS_REGISTERS_H
#define CODE_GEN_TESTS_REGISTERS_H
#include <cstdint>

enum class X64Register : uint8_t {
    RAX = 0b000,
    RCX = 0b001,
    RDX = 0b010,
    RBX = 0b011, //Nonvolatile
    RSP = 0b100, //Nonvolatile
    RBP = 0b101, //Nonvolatile
    RSI = 0b110, //Nonvolatile
    RDI = 0b111, //Nonvolatile
    //Extended registers
    R8 =  0b1000,
    R9 =  0b1001,
    R10 = 0b1010,
    R11 = 0b1011,
    R12 = 0b1100,
    R13 = 0b1101,
    R14 = 0b1110,
    R15 = 0b1111,
};

static std::initializer_list ALL_X64Registers = {
    X64Register::RAX,
    X64Register::RCX,
    X64Register::RDX,
    X64Register::RBX,
    X64Register::RSP,
    X64Register::RBP,
    X64Register::RSI,
    X64Register::RDI,
    X64Register::R8,
    X64Register::R9,
    X64Register::R10,
    X64Register::R11,
    X64Register::R12,
    X64Register::R13,
    X64Register::R14,
    X64Register::R15,
};

enum class X64SSERegister : uint8_t {
    XMM0 = 0b000,
    XMM1 = 0b001,
    XMM2 = 0b010,
    XMM3 = 0b011,
    XMM4 = 0b100,
    XMM5 = 0b101,
    XMM6 = 0b110, //Nonvolatile
    XMM7 = 0b111,
    XMM8 = 0b1000,
    XMM9 = 0b1001,
};

enum class X64SSERegisterPrecision : uint8_t {
    SCALER_SINGLE_PRECISION,
    SCALER_DOUBLE_PRECISION,
};

#endif //CODE_GEN_TESTS_REGISTERS_H
