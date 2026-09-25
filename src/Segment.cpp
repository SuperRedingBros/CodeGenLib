#include "../include/Assembler.h"
#include "../include/Segment.h"

void Segment::PushToBuffer(Segment* segment, TBuffer* buffer) {
    buffer->pushByte(static_cast<uint8_t>(segment->kind));
    buffer->pushString(segment->segment_name);
    buffer->pushBufferWithSize(segment->buffer);
}

Segment *Segment::PopFromBuffer(const Assembler *assembler, TBuffer *buffer) {
    const auto kind = static_cast<SegmentType>(buffer->readByte());
    const auto name = buffer->readString();
    TBuffer segmentBuf;
    buffer->readBufferWithSizeToBuffer(segmentBuf);
    return new Segment{
        .buffer = segmentBuf,
        .kind = kind,
        .segment_name = name,
    };
}
