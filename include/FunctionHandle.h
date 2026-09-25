#ifndef CODE_GEN_TESTS_FUNCTIONHANDLE_H
#define CODE_GEN_TESTS_FUNCTIONHANDLE_H
#include <cstdint>
#include <fstream>

#include "TBuffer.h"

class Assembler;
class CodeBuffer;
struct Segment;

enum class FunctionHandleKind : uint8_t {
    NORMAL,
    FORWARD_REF,
    LINKING_REF
};

class FunctionHandle {
public:
    Assembler* owningAssembler = nullptr;
    size_t bufOffset = 0;
    FunctionHandleKind handleKind = FunctionHandleKind::NORMAL;
    std::string name;
    Segment* segment = nullptr;

    ~FunctionHandle() = default;

    [[nodiscard]] size_t getAddress() const;

    static void PushToBuffer(FunctionHandle* handle, TBuffer *buffer);

    static FunctionHandle* PopFromBuffer(Assembler* owner, TBuffer* buffer);
};


#endif //CODE_GEN_TESTS_FUNCTIONHANDLE_H
