#include "../include/Relocation.h"
#include "../include/CodeBuffer.h"

void CodeLabel::PushToBuffer(const CodeLabel *label, TBuffer *buffer) {
    buffer->pushString(label->name);
    buffer->pushSize(label->offsetInFunc);
}

CodeLabel * CodeLabel::PopFromBuffer(TBuffer *buffer) {
    const std::string name = buffer->readString();
    const size_t offset = buffer->readSize();
    return new CodeLabel{
        .name = name,
        .offsetInFunc = offset,
    };
}

void Relocation::Apply(uint8_t *lowestAddress) const {
    const size_t writeBase = this->base->segment_address;
    size_t target_address = 0;
    if (this->segment != nullptr) {
        target_address = this->segment->GetAddress();
    } else {
        target_address = this->base->segment_address;
    }
    const uint32_t targetOffset = target_address
        + (label ? label->offsetInFunc: 0)
        - static_cast<ptrdiff_t>((targetRelativeToOffset ? 0 : this->size + this->offset) + writeBase);
    const size_t writeOffset = this->offset;
    if (this->size == 1) {
        reinterpret_cast<uint8_t*>(writeBase + writeOffset)[0] = static_cast<uint8_t>(targetOffset & 0xFF);
    } else if (this->size == 4) {
        reinterpret_cast<uint8_t*>(writeBase + writeOffset)[0] = static_cast<uint8_t>(targetOffset & 0xFF);
        reinterpret_cast<uint8_t*>(writeBase + writeOffset)[1] = static_cast<uint8_t>(targetOffset>>8 & 0xFF);
        reinterpret_cast<uint8_t*>(writeBase + writeOffset)[2] = static_cast<uint8_t>(targetOffset>>16 & 0xFF);
        reinterpret_cast<uint8_t*>(writeBase + writeOffset)[3] = static_cast<uint8_t>(targetOffset>>24 & 0xFF);
    }
}

void Relocation::PushToBuffer(const Relocation* relocation, TBuffer* buffer) {
    buffer->pushSize(relocation->size);
    buffer->pushSize(relocation->offset);
    //TODO fix
}

Relocation* Relocation::PopFromBuffer(Assembler* owningAssembler, TBuffer* buffer) {
    size_t size = buffer->readSize();
    size_t offset = buffer->readSize();
    return new Relocation{static_cast<Segment*>(nullptr), offset, size};
}