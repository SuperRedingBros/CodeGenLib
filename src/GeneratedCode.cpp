#include "../include/GeneratedCode.h"

#include <memoryapi.h>
#include <processthreadsapi.h>
#include <stdexcept>

#include "utils.cpp"

#include "../include/Relocation.h"
#include "../include/Segment.h"
#include "../include/Async.h"

GeneratedCode * GeneratedCode::Create() {
    return new GeneratedCode();
}

void GeneratedCode::loadSegments(const std::vector<Segment *> &segments, const std::vector<const Relocation *> &relocations) {
    std::unordered_map<SegmentType, TBuffer> compactedSegments{};
    for (const auto segment: segments) {
        if (!compactedSegments.contains(segment->kind))
            compactedSegments[segment->kind] = {};
        compactedSegments[segment->kind].appendBuffer(segment->buffer);
    }
    uint8_t* lowestAddress = nullptr;
    for (auto seg_type : SegmentTypes) {
        auto &seg_buf = compactedSegments[seg_type];
        if (auto data = VirtualAlloc(nullptr, seg_buf.size(), MEM_COMMIT, PAGE_READWRITE)) {
            memcpy(data, seg_buf.bottom(), seg_buf.size());
            if (lowestAddress == nullptr || data < lowestAddress)
                lowestAddress = static_cast<uint8_t*>(data);
            generated_code_segments.push_back({
                .size = seg_buf.size(),
                .data = static_cast<uint8_t*>(data),
                .segmentType = seg_type,
                .addresses = new GeneratedCodeSegment::Addresses
            });
            size_t i = 0;
            for (const auto segment: segments) {
                if (segment->kind == seg_type) {
                    if (seg_type == SegmentType::CODE) {
                        generated_code_segments.back().addresses->functionAddresses[segment->segment_name]
                            = reinterpret_cast<const void(*)()>(reinterpret_cast<uintptr_t>(data) + i);
                    }

                    segment->SetAddress(reinterpret_cast<uintptr_t>(data) + i);
                    i += segment->buffer.size();
                }
            }
        }
    }
    for (const auto generated_code_segment: generated_code_segments) {
        if (generated_code_segment.segmentType == SegmentType::CODE) {
            for (const auto reloc: relocations) {
                reloc->Apply(lowestAddress);
            }
            break;
        }
    }
    for (const auto [size, data, segmentType, _]: generated_code_segments) {
        DWORD OldProtection {};
        if (segmentType == SegmentType::CODE) {
            if (!VirtualProtect(data, size,
           PAGE_EXECUTE_READ, &OldProtection))
                throw std::runtime_error("VirtualProtect failed to allow execution.");
        }
        else if (segmentType == SegmentType::DATA) {
            if (!VirtualProtect(data, size,
           PAGE_READONLY, &OldProtection))
                throw std::runtime_error("VirtualProtect failed to mark segment as read-only.");
        }
    }

    for (const auto seg: segments) {
        if (seg->segment_name == "runtime_info") {
            const auto pFunctions = reinterpret_cast<PRUNTIME_FUNCTION>(seg->segment_address);
            RtlAddFunctionTable(pFunctions, 1, reinterpret_cast<DWORD64>(lowestAddress));
        }
    }
}

void GeneratedCode::dumpSegmentBytes(const size_t segmentId = 0) const {
    const auto&[size, data, _, _] = generated_code_segments[segmentId];
    hexDump(data, size);
}

void GeneratedCode::dumpAllSegmentsBytes() const {
    for (int i = 0; i < generated_code_segments.size(); ++i) {
        switch (generated_code_segments[i].segmentType) {
            case SegmentType::DATA:
                printf("Segment %d DATA:\n", i);
                dumpSegmentBytes(i);
                break;
            case SegmentType::VARIABLES:
                printf("Segment %d VARIABLES:\n", i);
                dumpSegmentBytes(i);
                break;
            case SegmentType::CODE:
                printf("Segment %d CODE:\n", i);
                dumpSegmentBytes(i);
        }
    }
}

const void (*GeneratedCode::getFuncAddress(const std::string &name) const)() {
    for (const auto segment: generated_code_segments) {
        if (segment.segmentType == SegmentType::CODE && segment.addresses->functionAddresses.contains(name)) {
            return segment.addresses->functionAddresses[name];
        }
    }
    return nullptr;
}

TCoroutine * GeneratedCode::createCoroutine(const std::string& function) const {
    return TCoroutine::Create(reinterpret_cast<void*>(getFunction<void>(function)));
}
