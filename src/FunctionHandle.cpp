#include "../include/FunctionHandle.h"

#include "../include/Assembler.h"
#include "../include/Segment.h"

size_t FunctionHandle::getAddress() const {
    return segment->segment_address;
}

void FunctionHandle::PushToBuffer(FunctionHandle* handle, TBuffer *buffer) {
    buffer->pushSize(handle->bufOffset);
    buffer->pushInt8(static_cast<uint8_t>(handle->handleKind));
    buffer->pushString(handle->segment->segment_name);
}

FunctionHandle* FunctionHandle::PopFromBuffer(Assembler *owner, TBuffer*buffer) {
    const auto buf_offset = buffer->readSize();
    const auto handleKind = static_cast<FunctionHandleKind>(buffer->readInt8());
    const auto segment_name = buffer->readString();

    const auto handle = new FunctionHandle(owner, buf_offset, handleKind);
    handle->segment = owner->GetSegment(segment_name);
    return handle;
}
