#ifndef CODE_GEN_TESTS_GENERATEDCODE_H
#define CODE_GEN_TESTS_GENERATEDCODE_H
#include <cstdint>

#include "FunctionHandle.h"
#include "Relocation.h"
#include "Segment.h"

struct TCoroutine;

class GeneratedCode {
    struct GeneratedCodeSegment {
        size_t size{};
        uint8_t* data = nullptr;
        SegmentType segmentType{};
        union Addresses {
            std::unordered_map<std::string, const void(*)()> functionAddresses{};
            std::unordered_map<std::string, const void*> dataAddresses;
            std::unordered_map<std::string, void*> variableAddresses;
        }* addresses{};
    };
    std::vector<GeneratedCodeSegment> generated_code_segments;
    GeneratedCode() = default;
public:
    static GeneratedCode* Create();

    void loadSegments(const std::vector<Segment *> &segments, const std::vector<const Relocation *> &relocations);

    void dumpSegmentBytes(size_t segmentId) const;

    void dumpAllSegmentsBytes() const;

    [[nodiscard]] const void (*getFuncAddress(const std::string &name) const)();

    template <typename RT, typename... Args>
    RT(*getFunction(const std::string &function) const)(Args...) {
        return reinterpret_cast<RT(*)(Args...)>(getFuncAddress(function));
    }

    template <typename RT, typename... Args>
    RT runFunction(const std::string& function, Args... args) {
        return getFunction<RT, Args...>(function)(args...);
    }

    [[nodiscard]] TCoroutine* createCoroutine(const std::string &function) const;
};

#endif //CODE_GEN_TESTS_GENERATEDCODE_H
