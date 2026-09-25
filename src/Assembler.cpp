#include "../include/Assembler.h"

#include <functional>
#include <ranges>
#include <stdexcept>

#include "../include/FunctionHandle.h"
#include "../include/GeneratedCode.h"
#include "../include/LinkingInfo.h"
#include "../include/Relocation.h"

FunctionHandle* Assembler::ForwardRef(const std::string_view name) {
    segments.push_back(new Segment{
        .kind = SegmentType::CODE
    });
    auto* functionHandle = new FunctionHandle{
        .owningAssembler = this,
        .handleKind = FunctionHandleKind::FORWARD_REF,
        .name = std::string(name),
        .segment = segments.back(),
    };
    functionHandles[std::string(name)] = functionHandle;
    return functionHandle;
}

FunctionHandle* Assembler::CreateFunction(const std::string_view name, const std::function<void(CodeBuffer*)> &functionMaker) {
    FunctionHandle* functionHandle = GetFunction(std::string(name));
    if (functionHandle != nullptr) {
        if (functionHandle->handleKind == FunctionHandleKind::NORMAL)
            throw std::runtime_error("Function already exists");
        functionHandle->handleKind = FunctionHandleKind::NORMAL;
    }
    else {
        segments.push_back(new Segment{
            .kind = SegmentType::CODE,
            .segment_name = std::string(name)
        });
        functionHandle = new FunctionHandle{
            .owningAssembler = this,
            .name = std::string(name),
            .segment = segments.back(),
        };
        functionHandles[std::string(name)] = functionHandle;
    }
    CodeBuffer funcBuf(this, functionHandle);
    const auto start_label = funcBuf.ReserveLabel("func_start");
    funcBuf.markLabel(start_label);
    functionMaker(&funcBuf);
    const auto end_label = funcBuf.ReserveLabel("func_end");
    funcBuf.markLabel(end_label);
    functionHandle->segment->buffer = *funcBuf.bufferRef();
    needsSegmentUpdate = 1;
    if (auto runtimeInfo = GetSegment("runtime_info")) {
        addRelocation(new Relocation(runtimeInfo, runtimeInfo->buffer.size(), 4, functionHandle, start_label));
        runtimeInfo->buffer.pushInt32(0);
        addRelocation(new Relocation(runtimeInfo, runtimeInfo->buffer.size(), 4, functionHandle, end_label));
        runtimeInfo->buffer.pushInt32(0);
        runtimeInfo->buffer.pushInt32(runtimeInfo->buffer.size() + 4);
        runtimeInfo->buffer.pushByte(0b001 << 0 | (0x2 | 0x1) << 3);
        runtimeInfo->buffer.pushByte(funcBuf.prologSize);
        runtimeInfo->buffer.pushByte(3);
        runtimeInfo->buffer.pushByte(static_cast<uint8_t>(X64Register::RBP) << 0 | 0 << 4);
        {// Prolog ops
            runtimeInfo->buffer.pushInt8(9); // offset
            runtimeInfo->buffer.pushInt8((3) | (0x0 << 4)); // op << 4 | opinfo
            runtimeInfo->buffer.pushInt8(6); // offset
            runtimeInfo->buffer.pushInt8((2) | (((funcBuf.reservedStack - 8) / 8) << 4)); // op << 4 | opinfo
            runtimeInfo->buffer.pushInt8(2); // offset
            runtimeInfo->buffer.pushInt8((0) | (static_cast<uint8_t>(X64Register::RBP) << 4)); // op << 4 | opinfo
            runtimeInfo->buffer.pushInt16(0); //padding
        }
        // addRelocation(new Relocation(runtimeInfo, runtimeInfo->buffer.size(), 4, runtimeInfo,
        //     LinkingInfo::Standard()->GetLinkedFunction(this, "on_except")));
        runtimeInfo->buffer.pushInt32(0);
    }
    return functionHandle;
}

void Assembler::AssembleAll(GeneratedCode* generated_code) const {
    generated_code->loadSegments(segments, relocations);
}

void Assembler::addRelocation(const Relocation *relocation) {
    relocations.push_back(relocation);
    needsRelocUpdate = 1;
}

void Assembler::PushToBuffer(TBuffer &buffer) const {
    buffer.pushByte(supportsAsync << 1 | needsRelocUpdate);

    buffer.pushArray<CodeLabel*>(labels, CodeLabel::PushToBuffer);
    buffer.pushArray<Segment*>(segments, Segment::PushToBuffer);
    buffer.pushMap<std::string, FunctionHandle*>(functionHandles, [](const std::string_view s, TBuffer* buf) -> void {
        buf->pushString(s);
    }, FunctionHandle::PushToBuffer);
    buffer.pushArray<const Relocation*>(relocations, Relocation::PushToBuffer);
}

void Assembler::PopFromBuffer(TBuffer &buffer) {
    const auto flags = buffer.readByte();
    supportsAsync = (flags >> 1) & 1;
    needsRelocUpdate = flags & 1;
    const auto readLabels = buffer.readArray<CodeLabel*>([](TBuffer* buf) -> CodeLabel* {
        return CodeLabel::PopFromBuffer(buf);
    });
    labels.clear();
    labels.insert(labels.end(), readLabels.begin(), readLabels.end());
    const auto in_segments = buffer.readArray<Segment*>([this](TBuffer* buf) -> Segment* {
        return Segment::PopFromBuffer(this, buf);
    });
    segments.clear();
    segments.insert(segments.end(), in_segments.begin(), in_segments.end());
    const auto functions = buffer.readMap<std::string, FunctionHandle*>(
        [](TBuffer* buf) -> std::string {return buf->readString();},
        [this](TBuffer* buf) -> FunctionHandle* {return FunctionHandle::PopFromBuffer(this, buf);});
    functionHandles.clear();
    for (const auto&[fst, snd]: functions) {
        functionHandles[fst] = snd;
    }
    const auto array = buffer.readArray<Relocation*>([this](TBuffer* buf) -> Relocation* {
        return Relocation::PopFromBuffer(this, buf);
    });
    relocations.clear();
    relocations.insert(relocations.end(), array.begin(), array.end());
}

FunctionHandle *Assembler::GetFunction(const std::string &str) const {
    const auto v = functionHandles.find(str);
    if (v == functionHandles.end()) {
        return nullptr;
    }
    return v->second;
}

Segment * Assembler::GetSegment(const std::string &str) const {
    for (const auto segment: segments) {
        if (segment->segment_name == str) {
            return segment;
        }
    }
    return nullptr;
}

Assembler::~Assembler() {
    for (const auto &snd: functionHandles | std::ranges::views::values) {
        delete snd;
    }
    for (const auto &reloc: relocations) {
        delete reloc;
    }
}
