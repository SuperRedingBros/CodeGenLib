#include "../include/TBuffer.h"

#include <ranges>

void TBuffer::pushChar(const int8_t byte) {
    data.push_back(byte);
}

void TBuffer::pushByte(const uint8_t byte) {
    pushChar(static_cast<int8_t>(byte));
}

void TBuffer::pushInt16(const uint16_t number) {
    if (isLittleEndian) {
        pushByte(number & 0xFF);
        pushByte((number >> 8) & 0xFF);
    } else {
        pushByte((number >> 8) & 0xFF);
        pushByte(number & 0xFF);
    }
}

void TBuffer::pushInt32(const uint32_t number) {
    if (isLittleEndian) {
        pushInt16(number & 0xFFFF);
        pushInt16((number >> 16) & 0xFFFF);
    } else {
        pushInt16((number >> 16) & 0xFFFF);
        pushInt16(number & 0xFFFF);
    }
}

void TBuffer::pushInt64(const uint64_t number) {
    if (isLittleEndian) {
        pushInt32(number & 0xFFFFFFFF);
        pushInt32((number >> 32) & 0xFFFFFFFF);
    } else {
        pushInt32((number >> 32) & 0xFFFFFFFF);
        pushInt32(number & 0xFFFFFFFF);
    }
}

void TBuffer::pushFloat(const float number) {
    char c[sizeof(float)];
    memcpy(c, &number, sizeof(float));
    if (isLittleEndian) {
        for (const char i : c) {
            pushByte(i);
        }
    } else {
        for (const char i : c | std::views::reverse) {
            pushByte(i);
        }
    }
}

void TBuffer::pushDouble(const double number) {
    char c[sizeof(double)];
    memcpy(c, &number, sizeof(double));
    if (isLittleEndian) {
        for (const char i : c) {
            pushByte(i);
        }
    } else {
        for (const char i : c | std::views::reverse) {
            pushByte(i);
        }
    }
}

int8_t TBuffer::readChar() {
    const auto v = *((*this)[0]);
    head++;
    return v;
}

uint8_t TBuffer::readByte() {
    return readChar();
}

uint16_t TBuffer::readInt16() {
    if (isLittleEndian)
        return readByte() | readByte() << 8;
    return readByte() << 8 | readByte();
}

uint32_t TBuffer::readInt32() {
    if (isLittleEndian)
        return readInt16() | readInt16() << 16;
    return readInt16() << 16 | readInt16();
}

uint64_t TBuffer::readInt64() {
    if (isLittleEndian)
        return readInt32() | static_cast<uint64_t>(readInt32()) << 32;
    return static_cast<uint64_t>(readInt32()) << 32 | readInt32();
}

float TBuffer::readFloat() {
    char c[sizeof(float)];
    float number = 0;
    if (isLittleEndian) {
        for (char& i : c) {
            i = readChar();
        }
    } else {
        for (char& i : c | std::views::reverse) {
            i = readChar();
        }
    }
    memcpy(&number, c, sizeof(float));
    return number;
}

double TBuffer::readDouble() {
    char c[sizeof(double)];
    double number;
    if (isLittleEndian) {
        for (char& i : c) {
            i = readChar();
        }
    } else {
        for (char& i : c | std::views::reverse) {
            i = readChar();
        }
    }
    memcpy(&number, c, sizeof(double));
    return number;
}
