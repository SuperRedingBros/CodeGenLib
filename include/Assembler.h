#ifndef CODE_GEN_TESTS_ASSEMBLER_H
#define CODE_GEN_TESTS_ASSEMBLER_H
#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "FunctionHandle.h"
#include "Relocation.h"
#include "CodeBuffer.h"

#include "GeneratedCode.h"
#include "Segment.h"

class Assembler {
    std::unordered_map<std::string, FunctionHandle*> functionHandles{};
    std::vector<const Relocation*> relocations{};
    std::vector<CodeLabel*> labels{};

    std::vector<Segment*> segments{};

    std::uint8_t supportsAsync: 1 = 0;
    std::uint8_t needsRelocUpdate: 1 = 0;
    std::uint8_t needsSegmentUpdate: 1 = 0;
public:
    ~Assembler();

    FunctionHandle* CreateFunction(std::string_view name, const std::function<void(CodeBuffer*)> &functionMaker);

    void AssembleAll(GeneratedCode *generated_code) const;

    FunctionHandle* ForwardRef(std::string_view name);

    CodeLabel* ReserveLabel(const std::string_view name) {
        auto* l = new CodeLabel {
            .name = std::string(name),
        };
        labels.push_back(l);
        return l;
    }

    const CodeLabel* GetLabel(const std::string name) const {
        for (const auto* l : labels) {
            if (l->name == name) {
                return l;
            }
        }
        return nullptr;
    }

    Segment* AllocSegment(const SegmentType segmentType, const std::function<void(TBuffer *)> &consumer) {
        return AllocSegment(segmentType, "data_segment_" + std::to_string(segments.size()), consumer);
    }

    Segment* AllocSegment(const SegmentType segmentType, const std::string& name, const std::function<void(TBuffer *)> &consumer) {
        segments.push_back(new Segment{
            .kind = segmentType,
            .segment_name = name,
        });
        consumer(&segments.back()->buffer);
        return segments.back();
    }

    void addRelocation(const Relocation* relocation);

    void PushToBuffer(TBuffer& buffer) const;

    void PopFromBuffer(TBuffer& buffer);

    FunctionHandle *GetFunction(const std::string &str) const;
    Segment *GetSegment(const std::string &str) const;
};


#endif //CODE_GEN_TESTS_ASSEMBLER_H
