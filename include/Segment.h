#ifndef CODE_GEN_TESTS_SEGMENT_H
#define CODE_GEN_TESTS_SEGMENT_H
#include "TBuffer.h"

enum class SegmentType : uint8_t {
    DATA, VARIABLES, CODE
};

constexpr auto SegmentTypes = {SegmentType::DATA, SegmentType::VARIABLES, SegmentType::CODE};

struct Segment {
    TBuffer buffer;
    SegmentType kind = SegmentType::DATA;
    uintptr_t segment_address;
    std::string segment_name;

    void SetAddress(const size_t address) {
        segment_address = address;
    }

    [[nodiscard]] size_t GetAddress() const {
        return segment_address;
    }

    static void PushToBuffer(Segment *segment, TBuffer *buffer);

    static Segment *PopFromBuffer(const Assembler *assembler, TBuffer *buffer);
};

#endif //CODE_GEN_TESTS_SEGMENT_H
