#include <filesystem>
#include <functional>
#include <iostream>
#include <ranges>

#include "../include/Assembler.h"
#include "../include/CodeBuffer.h"
#include "../include/GeneratedCode.h"
#include "../include/LinkingInfo.h"


int main() {
    // Assembler assembler;
    // auto seg = assembler.AllocSegment(SegmentType::DATA, "array", [](TBuffer* buffer) -> void {
    //     buffer->pushDouble(32.1);
    // });
    // auto reg = X64Register::RAX;
    // assembler.CreateFunction("call_test", [seg, reg](CodeBuffer* buf) -> void {
    //             buf->prolog(0x20, {X64Register::RBX});
    //
    //             buf->movReg(X64Register::R9, 32);
    //             buf->movReg(X64Register::R8, 4);
    //             buf->divReg(X64Register::RAX, X64Register::RDX, X64Register::R9, X64Register::R8);
    //
    //             buf->epilog();
    //             buf->ret();
    // });
    // GeneratedCode* code = GeneratedCode::Create();
    // assembler.AssembleAll(code);
    // code->dumpAllSegmentsBytes();
    // const auto v = code->getFunction<int>("call_test");
    // printf("%i\n", v());
    const std::string s = "#include <gtest/gtest.h>\n";
    const std::filesystem::path pth = std::filesystem::current_path().parent_path() / "tests" / "tests.cpp";
    TBuffer buffer(pth);
    const std::string s1 = buffer.readLine();
    std::cout << s << std::endl;
    std::cout << s1 << std::endl;
    std::cout << std::filesystem::current_path() << std::endl;

    return 0;
}
