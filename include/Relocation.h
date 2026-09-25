#ifndef CODE_GEN_TESTS_RELOCATION_H
#define CODE_GEN_TESTS_RELOCATION_H

#include <string>
#include "FunctionHandle.h"
#include "Segment.h"
#include "TBuffer.h"

struct CodeLabel {
    const std::string name;
    size_t offsetInFunc;

    static void PushToBuffer(const CodeLabel* label, TBuffer* buffer);

    static CodeLabel* PopFromBuffer(TBuffer *buffer);
};

class Relocation {
public:
    Relocation(const Segment* base, const size_t offset, const size_t size)
        : base(base),
          offset(offset),
          size(size),
          label(nullptr),
          segment(nullptr) {}
    Relocation(const Segment* base, const size_t offset, const size_t size, const Segment* segment)
        : base(base),
          offset(offset),
          size(size),
          label(nullptr),
          segment(segment) {}
    Relocation(const FunctionHandle* base, const size_t offset, const size_t size)
        : base(base->segment),
          offset(offset),
          size(size),
          label(nullptr),
          segment(nullptr) {}
    Relocation(const Segment* base, const size_t offset, const size_t size, const FunctionHandle *forward_ref)
        : base(base),
          offset(offset),
          size(size),
          label(nullptr),
          segment(forward_ref->segment) {}
    Relocation(const FunctionHandle* base, const size_t offset, const size_t size, const FunctionHandle *forward_ref)
        : base(base->segment),
          offset(offset),
          size(size),
          label(nullptr),
          segment(forward_ref->segment) {}
    Relocation(const Segment* base, const size_t offset, const size_t size, const FunctionHandle *forward_ref, const CodeLabel *label_ref)
        : base(base),
          offset(offset),
          size(size),
          label(label_ref),
          segment(forward_ref->segment) {}
    Relocation(const Segment* base, const size_t offset, const size_t size, const CodeLabel *label_ref)
        : base(base),
          offset(offset),
          size(size),
          label(label_ref),
          segment(nullptr) {}
    Relocation(const FunctionHandle* base, const size_t offset, const size_t size, const CodeLabel *label_ref)
        : base(base->segment),
          offset(offset),
          size(size),
          label(label_ref),
          segment(nullptr) {}
    Relocation(const FunctionHandle* base, const size_t offset, const size_t size, const Segment *segment)
        : base(base->segment),
          offset(offset),
          size(size),
          label(nullptr),
          segment(segment) {}
    Relocation(const Segment* base, const size_t offset, const size_t size, const Segment *segment, const CodeLabel *label)
        : base(base),
          offset(offset),
          size(size),
          label(label),
          segment(segment) {}
    Relocation(const FunctionHandle* base, const size_t offset, const size_t size, const Segment *segment, const CodeLabel *label)
        : base(base->segment),
          offset(offset),
          size(size),
          label(label),
          segment(segment) {}
    ~Relocation() = default;

    void Apply(uint8_t* lowestAddress) const;

    static void PushToBuffer(const Relocation* relocation, TBuffer* buffer);

    static Relocation* PopFromBuffer(Assembler* owningAssembler, TBuffer* buffer);
protected:
    const Segment* base;
    const size_t offset;
    const size_t size;
    const CodeLabel* label;
    const Segment* segment;
    const uint8_t targetRelativeToOffset: 1 = 0;
};

#endif //CODE_GEN_TESTS_RELOCATION_H
