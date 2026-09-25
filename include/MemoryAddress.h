#ifndef CODE_GEN_TESTS_MEMORYADDRESS_H
#define CODE_GEN_TESTS_MEMORYADDRESS_H

enum class X64MemoryAddressSIBScale : uint8_t {
    NONE, BYTE, TWO_BYTES, FOUR_BYTES, EIGHT_BYTES,
};

struct X64MemoryAddress {
    int64_t displacement{};
    X64Register base{};
    X64MemoryAddressSIBScale scale{X64MemoryAddressSIBScale::NONE};
    X64Register index{};

    [[nodiscard]] uint8_t makeSIBByte() const {
        if (index == X64Register::RSP)
            throw std::invalid_argument("Scaled index may not be RSP");
        if (base == X64Register::RBP)
            throw std::invalid_argument("Base may not be RBP");
        return ((static_cast<uint8_t>(scale)-1) & 0b11) << 6 |
                (static_cast<uint8_t>(index) & 0b111) << 3 |
                (static_cast<uint8_t>(base) & 0b111) << 0;
    }

    [[nodiscard]] std::vector<uint8_t> makeMemoryAddressingBytes(const X64Register reg) const {
        return makeMemoryAddressingBytes(static_cast<uint8_t>(reg));
    }

    [[nodiscard]] std::vector<uint8_t> makeMemoryAddressingBytes(const X64SSERegister reg) const {
        return makeMemoryAddressingBytes(static_cast<uint8_t>(reg));
    }

    [[nodiscard]] std::vector<uint8_t> makeMemoryAddressingBytes(const uint8_t reg) const {
        if (scale != X64MemoryAddressSIBScale::NONE) {
            if (displacement == 0) {
                if (base == X64Register::RBP) {
                    return {
                        makeMod_RM_Byte(Mod_RM_AddressMode::INDIRECT_8Bit_DISPLACEMENT, reg, 0b100),
                        makeSIBByte(),
                        0x00
                    };
                }
                return {
                    makeMod_RM_Byte(Mod_RM_AddressMode::INDIRECT_NO_DISPLACEMENT, reg, 0b100),
                    makeSIBByte(),
                };
            }
            return {
                makeMod_RM_Byte(Mod_RM_AddressMode::INDIRECT_32Bit_DISPLACEMENT, reg, 0b100),
                makeSIBByte(),
                static_cast<uint8_t>(displacement & 0xFF),
                static_cast<uint8_t>(displacement >> 8 & 0xFF),
                static_cast<uint8_t>(displacement >> 16 & 0xFF),
                static_cast<uint8_t>(displacement >> 24 & 0xFF),
            };
        }
        if (displacement == 0) {
            if (base == X64Register::RBP) {
                return {
                    makeMod_RM_Byte(Mod_RM_AddressMode::INDIRECT_8Bit_DISPLACEMENT, reg, static_cast<uint8_t>(base)),
                    0x00
                };
            }
            return {
                makeMod_RM_Byte(Mod_RM_AddressMode::INDIRECT_NO_DISPLACEMENT, reg, static_cast<uint8_t>(base)),
            };
        }
        return {
            makeMod_RM_Byte(Mod_RM_AddressMode::INDIRECT_32Bit_DISPLACEMENT, reg, static_cast<uint8_t>(base)),
            static_cast<uint8_t>(displacement & 0xFF),
            static_cast<uint8_t>(displacement >> 8 & 0xFF),
            static_cast<uint8_t>(displacement >> 16 & 0xFF),
            static_cast<uint8_t>(displacement >> 24 & 0xFF),
        };
    }
};

#endif //CODE_GEN_TESTS_MEMORYADDRESS_H
