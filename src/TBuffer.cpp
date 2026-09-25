#include "../include/TBuffer.h"

void TBuffer::pushChar(const int8_t byte) {
    data.push_back(byte);
}

void TBuffer::pushByte(const uint8_t byte) {
    pushChar(static_cast<int8_t>(byte));
}

void TBuffer::pushInt16(const uint16_t number) {
    pushByte(number & 0xFF);
    pushByte((number >> 8) & 0xFF);
}

void TBuffer::pushInt32(const uint32_t number) {
    pushByte(number & 0xFF);
    pushByte((number >> 8) & 0xFF);
    pushByte((number >> 16) & 0xFF);
    pushByte((number >> 24) & 0xFF);
}

void TBuffer::pushInt64(const uint64_t number) {
    pushByte(number & 0xFF);
    pushByte((number >> 8) & 0xFF);
    pushByte((number >> 16) & 0xFF);
    pushByte((number >> 24) & 0xFF);
    pushByte((number >> 32) & 0xFF);
    pushByte((number >> 40) & 0xFF);
    pushByte((number >> 48) & 0xFF);
    pushByte((number >> 56) & 0xFF);
}

void TBuffer::pushFloat(const float number) {
    char c[sizeof(float)];
    memcpy(c, &number, sizeof(float));
    for (const char i : c) {
        pushByte(i);
    }
}

void TBuffer::pushDouble(const double number) {
    char c[sizeof(double)];
    memcpy(c, &number, sizeof(double));
    for (const char i : c) {
        pushByte(i);
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
    return readByte() | readByte() << 8;
}

uint32_t TBuffer::readInt32() {
    return readInt16() | readInt16() << 16;
}

uint64_t TBuffer::readInt64() {
    return readInt32() | static_cast<uint64_t>(readInt32()) << 32;
}
