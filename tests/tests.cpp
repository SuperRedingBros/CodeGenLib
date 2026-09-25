#include <gtest/gtest.h>

#include "../include/Assembler.h"
#include "../include/GeneratedCode.h"
#include "../include/LinkingInfo.h"
#include "../include/Async.h"

// Used by LinkingTest
static int receivedValueFromLinkTest;
namespace {
    class RegisterMoveTest : public ::testing::Test {
    public:
        static void TryRegister(const X64Register reg) {
            Assembler assembler;
            const auto reg_test = assembler.CreateFunction("reg_test", [reg](CodeBuffer* buf) -> void {
                buf->prolog(0x20, {X64Register::RBX});

                buf->movReg(reg, 20);
                buf->movReg(X64Register::RAX, reg);

                buf->epilog();
                buf->ret();
            });
            GeneratedCode* code = GeneratedCode::Create();
            assembler.AssembleAll(code);
            EXPECT_EQ(code->runFunction<int>(reg_test->name), 20);
        }
    };
    class XmmRegisterMoveTest : public ::testing::Test {
    public:
        static void TryRegister(const X64SSERegister reg) {
            Assembler assembler;
            const auto seg = assembler.AllocSegment(SegmentType::DATA, "floats", [](auto buf) -> void {
                buf->pushDouble(3.145);
            });
            const auto reg_test = assembler.CreateFunction("reg_test", [seg, reg](CodeBuffer* buf) -> void {
                buf->prolog(0x20, {});

                buf->movReg(reg, seg);
                buf->movReg(X64SSERegister::XMM0, reg);

                buf->epilog();
                buf->ret();
            });
            GeneratedCode* code = GeneratedCode::Create();
            assembler.AssembleAll(code);
            EXPECT_NEAR(code->runFunction<double>(reg_test->name), 3.145, .000001);
        }
    };
    class RegisterMoveSIBTest : public ::testing::Test {
    public:
        static void TryRegister(const X64Register reg, const X64Register reg1) {
            Assembler assembler;
            auto seg = assembler.AllocSegment(SegmentType::VARIABLES, "array", [](TBuffer* buffer) -> void {
                buffer->pushInt64(13);
                buffer->pushInt64(21);
                buffer->pushInt64(123);
                buffer->pushInt64(432);
            });
            assembler.CreateFunction("reg_test", [seg, reg, reg1](CodeBuffer* buf) -> void {
                buf->prolog(0x20, {X64Register::RBX});

                buf->leaReg(reg, seg);
                buf->movReg(reg1, 2);
                buf->movReg(X64Register::RAX, {
                    .base = reg,
                    .scale = X64MemoryAddressSIBScale::EIGHT_BYTES,
                    .index = reg1
                }, RegSize::X64);
                buf->movReg(reg1, 3);
                buf->movReg({
                    .base = reg,
                    .scale = X64MemoryAddressSIBScale::EIGHT_BYTES,
                    .index = reg1
                }, X64Register::RAX, RegSize::X64);

                buf->epilog();
                buf->ret();
            });
            assembler.CreateFunction("reg_test2", [seg, reg, reg1](CodeBuffer* buf) -> void {
                buf->prolog(0x20, {X64Register::RBX});

                buf->leaReg(reg, seg);
                buf->movReg(reg1, 3);
                buf->movReg(X64Register::RAX, {
                    .base = reg,
                    .scale = X64MemoryAddressSIBScale::EIGHT_BYTES,
                    .index = reg1
                }, RegSize::X64);

                buf->epilog();
                buf->ret();
            });
            GeneratedCode* code = GeneratedCode::Create();
            assembler.AssembleAll(code);
            EXPECT_EQ(code->runFunction<int>("reg_test"), 123);
            EXPECT_EQ(code->runFunction<int>("reg_test2"), 123);
        }
    };
    class XmmRegisterMemoryMoveTest : public ::testing::Test {
    public:
        static void TryRegister(const X64SSERegister reg) {
            Assembler assembler;
            const auto seg = assembler.AllocSegment(SegmentType::VARIABLES, "floats", [](auto buf) -> void {
                buf->pushDouble(3.145);
                buf->pushDouble(2.145);
                buf->pushDouble(1.145);
            });
            const auto reg_test = assembler.CreateFunction("reg_test", [seg, reg](CodeBuffer* buf) -> void {
                buf->prolog(0x20, {});

                buf->leaReg(X64Register::R8, seg);
                buf->movReg(X64SSERegister::XMM0, {
                    .base = X64Register::R8,
                });

                buf->epilog();
                buf->ret();
            });
            const auto reg_test1 = assembler.CreateFunction("reg_test1", [seg](CodeBuffer* buf) -> void {
                buf->prolog(0x20, {});

                buf->leaReg(X64Register::R8, seg);
                buf->movReg(X64Register::R9, 2);
                buf->movReg(X64SSERegister::XMM0, {
                    .base = X64Register::R8,
                    .scale = X64MemoryAddressSIBScale::EIGHT_BYTES,
                    .index = X64Register::R9,
                });

                buf->epilog();
                buf->ret();
            });
            GeneratedCode* code = GeneratedCode::Create();
            assembler.AssembleAll(code);
            EXPECT_NEAR(code->runFunction<double>(reg_test->name),  3.145, .000001);
            EXPECT_NEAR(code->runFunction<double>(reg_test1->name), 1.145, .000001);
        }
    };
    class RegisterMathTest : public ::testing::Test {
    public:
        static void TryRegister(const X64Register reg) {
            Assembler assembler;
            const auto reg_test = assembler.CreateFunction("reg_test", [reg](CodeBuffer* buf) -> void {
                buf->prolog(0x20, {X64Register::RBX});

                buf->movReg(reg, 20);
                buf->addReg(reg, 32);
                buf->subReg(reg, 50);
                buf->addReg(reg, 56);
                buf->subReg(reg, 2);
                buf->movReg(X64Register::RAX, reg);
                buf->movReg(X64Register::R8, 4);
                buf->mulReg(X64Register::RAX, X64Register::RDX, reg, X64Register::R8);
                buf->negReg(X64Register::RAX);

                buf->epilog();
                buf->ret();
            });
            GeneratedCode* code = GeneratedCode::Create();
            assembler.AssembleAll(code);
            EXPECT_EQ(code->runFunction<int>(reg_test->name), -(20 + 32 - 50 + 56 - 2) * 4);
        }
    };
    class XmmRegisterMathTest : public ::testing::Test {
    public:
        static void TryRegister(const X64SSERegister reg, const X64SSERegister reg1) {
            Assembler assembler;
            const auto seg = assembler.AllocSegment(SegmentType::DATA, "floats", [](auto buf) -> void {
                buf->pushDouble(3.145);
            });
            const auto seg1 = assembler.AllocSegment(SegmentType::DATA, "floats", [](auto buf) -> void {
                buf->pushDouble(1.145);
            });
            const auto add_test = assembler.CreateFunction("add_test", [seg, seg1, reg, reg1](CodeBuffer* buf) -> void {
                buf->prolog(0x20, {});

                buf->movReg(reg, seg);
                buf->movReg(reg1, seg1);
                buf->addReg(reg, reg1);

                buf->epilog();
                buf->ret();
            });
            const auto sub_test = assembler.CreateFunction("sub_test", [seg, seg1, reg, reg1](CodeBuffer* buf) -> void {
                buf->prolog(0x20, {});

                buf->movReg(reg, seg);
                buf->movReg(reg1, seg1);
                buf->subReg(reg, reg1);

                buf->epilog();
                buf->ret();
            });
            const auto mul_test = assembler.CreateFunction("mul_test", [seg, seg1, reg, reg1](CodeBuffer* buf) -> void {
                buf->prolog(0x20, {});

                buf->movReg(reg, seg);
                buf->movReg(reg1, seg1);
                buf->mulReg(reg, reg1);

                buf->epilog();
                buf->ret();
            });
            const auto div_test = assembler.CreateFunction("div_test", [seg, seg1, reg, reg1](CodeBuffer* buf) -> void {
                buf->prolog(0x20, {});

                buf->movReg(reg, seg);
                buf->movReg(reg1, seg1);
                buf->divReg(reg, reg1);

                buf->epilog();
                buf->ret();
            });
            GeneratedCode* code = GeneratedCode::Create();
            assembler.AssembleAll(code);
            EXPECT_NEAR(code->runFunction<double>(add_test->name), 3.145 + 1.145, .000001);
            EXPECT_NEAR(code->runFunction<double>(sub_test->name), 3.145 - 1.145, .000001);
            EXPECT_NEAR(code->runFunction<double>(mul_test->name), 3.145 * 1.145, .000001);
            EXPECT_NEAR(code->runFunction<double>(div_test->name), 3.145 / 1.145, .000001);
        }
    };
    class RegisterPushTest : public ::testing::Test {
    public:
        static void TryRegister(const X64Register reg) {
            Assembler assembler;
            const auto reg_test = assembler.CreateFunction("reg_test", [reg](CodeBuffer* buf) -> void {
                buf->prolog(0x20, {X64Register::RBX});


                buf->movReg(reg, 123);
                buf->pushReg(reg);
                buf->movReg(reg, 2);
                buf->popReg(reg);


                buf->movReg(X64Register::RAX, reg);

                buf->epilog();
                buf->ret();
            });
            GeneratedCode* code = GeneratedCode::Create();
            assembler.AssembleAll(code);
            EXPECT_EQ(code->runFunction<int>(reg_test->name), 123);
        }
    };
    class RegisterMemoryTest : public ::testing::Test {
    public:
        static void TryRegister(const X64Register reg) {
            Assembler assembler;
            const auto reg_test = assembler.CreateFunction("reg_test", [reg](CodeBuffer* buf) -> void {
                buf->prolog(0x20, {});

                buf->movReg(reg, 123);
                buf->movReg({
                    .displacement = 8,
                    .base = X64Register::RBP
                }, reg);
                buf->clearReg(reg);
                buf->movReg(reg, {
                    .displacement = 8,
                    .base = X64Register::RBP
                });

                buf->movReg(X64Register::RAX, reg);

                buf->epilog();
                buf->ret();
            });
            GeneratedCode* code = GeneratedCode::Create();
            assembler.AssembleAll(code);
            EXPECT_EQ(code->runFunction<int>(reg_test->name), 123);
        }
    };
    class LinkingTest : public ::testing::Test {
    protected:
        LinkingInfo* linkingInfo{};
        static void ReceiveValue(const int i) {
            receivedValueFromLinkTest = i;
        }
        void SetUp() override {
            linkingInfo = new LinkingInfo();
            linkingInfo->AddLinkedFunction("link_test", ReceiveValue);
        };
        void TearDown() override {
            delete linkingInfo;
            receivedValueFromLinkTest = 0;
        }
    public:
        void SetupLinking(Assembler& assembler) const {
            linkingInfo->AllocForAssembler(assembler);
            linkingInfo->LinkAssembler(assembler);
        }
        static int CheckValue() {
            return receivedValueFromLinkTest;
        }
    };
}

TEST_F(RegisterMoveTest, RBXMove) {
    TryRegister(X64Register::RBX);
}
TEST_F(RegisterMoveTest, RDXMove) {
    TryRegister(X64Register::RDX);
}
TEST_F(RegisterMoveTest, RCXMove) {
    TryRegister(X64Register::RCX);
}
TEST_F(RegisterMoveTest, R8Move) {
    TryRegister(X64Register::R8);
}
TEST_F(RegisterMoveTest, R9Move) {
    TryRegister(X64Register::R9);
}

TEST_F(RegisterMoveSIBTest, RBXMove) {
    TryRegister(X64Register::RBX, X64Register::RDX);
}
TEST_F(RegisterMoveSIBTest, RDXMove) {
    TryRegister(X64Register::RDX, X64Register::RBX);
}
TEST_F(RegisterMoveSIBTest, RCXMove) {
    TryRegister(X64Register::RCX, X64Register::RDI);
}
TEST_F(RegisterMoveSIBTest, R8Move) {
    TryRegister(X64Register::R8, X64Register::RSI);
}
TEST_F(RegisterMoveSIBTest, R9Move) {
    TryRegister(X64Register::R9, X64Register::R8);
}

TEST_F(XmmRegisterMoveTest, XMM0Move) {
    TryRegister(X64SSERegister::XMM0);
}
TEST_F(XmmRegisterMoveTest, XMM1Move) {
    TryRegister(X64SSERegister::XMM1);
}
TEST_F(XmmRegisterMoveTest, XMM9Move) {
    TryRegister(X64SSERegister::XMM9);
}

TEST_F(XmmRegisterMemoryMoveTest, XMM0Move) {
    TryRegister(X64SSERegister::XMM0);
}
TEST_F(XmmRegisterMemoryMoveTest, XMM1Move) {
    TryRegister(X64SSERegister::XMM1);
}
TEST_F(XmmRegisterMemoryMoveTest, XMM9Move) {
    TryRegister(X64SSERegister::XMM9);
}

TEST(XmmGeneralMoveTest, GeneralToXmm) {
    Assembler assembler;
    const auto seg = assembler.AllocSegment(SegmentType::DATA, "floats", [](auto buf) -> void {
        buf->pushDouble(3.145);
    });
    const auto reg_test = assembler.CreateFunction("reg_test", [seg](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {});

        buf->movReg_mem(X64Register::RBX, seg);
        buf->movReg(X64SSERegister::XMM0, X64Register::RBX);

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    EXPECT_NEAR(code->runFunction<double>(reg_test->name), 3.145, .000001);
}
TEST(XmmGeneralMoveTest, XMMToGeneral) {
    Assembler assembler;
    const auto seg = assembler.AllocSegment(SegmentType::DATA, "floats", [](auto buf) -> void {
        buf->pushDouble(3.145);
    });
    const auto reg_test = assembler.CreateFunction("reg_test", [seg](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {});

        buf->movReg(X64SSERegister::XMM1, seg);
        buf->movReg(X64Register::RBX, X64SSERegister::XMM1);
        buf->movReg(X64SSERegister::XMM0, X64Register::RBX);

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    EXPECT_NEAR(code->runFunction<double>(reg_test->name), 3.145, .000001);
}

TEST_F(XmmRegisterMathTest, XMM1Math) {
    TryRegister(X64SSERegister::XMM0, X64SSERegister::XMM1);
}
TEST_F(XmmRegisterMathTest, XMM2Math) {
    TryRegister(X64SSERegister::XMM0, X64SSERegister::XMM2);
}

TEST_F(RegisterMathTest, RAXMathTest) {
    TryRegister(X64Register::RAX);
}
TEST_F(RegisterMathTest, RBXMathTest) {
    TryRegister(X64Register::RBX);
}
TEST_F(RegisterMathTest, RCXMathTest) {
    TryRegister(X64Register::RBX);
}
TEST_F(RegisterMathTest, R8MathTest) {
    TryRegister(X64Register::R8);
}
TEST_F(RegisterMathTest, R9MathTest) {
    TryRegister(X64Register::R9);
}

TEST_F(RegisterPushTest, RAXPushTest) {
    TryRegister(X64Register::RAX);
}
TEST_F(RegisterPushTest, RBXPushTest) {
    TryRegister(X64Register::RBX);
}
TEST_F(RegisterPushTest, RCXPushTest) {
    TryRegister(X64Register::RBX);
}
TEST_F(RegisterPushTest, R8PushTest) {
    TryRegister(X64Register::R8);
}
TEST_F(RegisterPushTest, R9PushTest) {
    TryRegister(X64Register::R9);
}

TEST_F(RegisterMemoryTest, RAXMemoryTest) {
    TryRegister(X64Register::RAX);
}
TEST_F(RegisterMemoryTest, RBXMemoryTest) {
    TryRegister(X64Register::RBX);
}
TEST_F(RegisterMemoryTest, RCXMemoryTest) {
    TryRegister(X64Register::RBX);
}
TEST_F(RegisterMemoryTest, R8MemoryTest) {
    TryRegister(X64Register::R8);
}
TEST_F(RegisterMemoryTest, R9MemoryTest) {
    TryRegister(X64Register::R9);
}

TEST(LabelJumpTest, LabelJumpReturn) {
    Assembler assembler;
    const auto jump_test = assembler.CreateFunction("jump_test", [](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->movReg(X64Register::RAX, 123);

        const auto l = buf->jumpToLabel();

        buf->movReg(X64Register::RAX, 0);

        buf->markLabel(l);

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    EXPECT_EQ(code->runFunction<int>(jump_test->name), 123);
}
TEST(LabelJumpTest, LabelJumpReserved) {
    Assembler assembler;
    const auto jump_test = assembler.CreateFunction("jump_test", [](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->movReg(X64Register::RAX, 123);

        auto l = buf->ReserveLabel("jump_test");
        buf->jumpToLabel(l);

        buf->movReg(X64Register::RAX, 0);

        buf->markLabel(l);

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    EXPECT_EQ(code->runFunction<int>(jump_test->name), 123);
}
TEST(LabelJumpTest, LabelJumpConditional) {
    Assembler assembler;
    const auto jump_test = assembler.CreateFunction("jump_test", [](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->movReg(X64Register::RAX, 123);

        const auto l = buf->ReserveLabel("jump_test");
        buf->cmpReg(X64Register::RAX, 100);
        buf->jumpToLabelCond(l, ConditionModes::GREATER);

        buf->movReg(X64Register::RAX, 0);

        buf->markLabel(l);

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    EXPECT_EQ(code->runFunction<int>(jump_test->name), 123);
}

TEST(FunctionTest, CallFunctions) {
    Assembler assembler;
    const auto call_test2 = assembler.ForwardRef("call_test2");
    const auto call_test = assembler.CreateFunction("call_test", [](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->movReg(X64Register::RAX, 20);

        buf->epilog();
        buf->ret();
    });
    const auto call_test1 = assembler.CreateFunction("call_test1", [call_test2, call_test](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->movReg(X64Register::RBX, 100);

        buf->callFunction(call_test);
        buf->addReg(X64Register::RBX, X64Register::RAX);

        buf->callFunction(call_test2);
        buf->addReg(X64Register::RBX, X64Register::RAX);


        buf->movReg(X64Register::RAX, X64Register::RBX);

        buf->epilog();
        buf->ret();
    });
    assembler.CreateFunction("call_test2", [](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->movReg(X64Register::RAX, 3);

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    EXPECT_EQ(code->runFunction<int>(call_test1->name), 123);
}

TEST(ConstantTests, LoadConstant) {
    Assembler assembler;
    const Segment* intVal = assembler.AllocSegment(SegmentType::DATA,[](TBuffer* b) -> void {
        b->pushInt64(123);
    });
    const auto call_test = assembler.CreateFunction("call_test", [intVal](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->movReg_mem(X64Register::RAX, intVal);

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    EXPECT_EQ(code->runFunction<int>(call_test->name), 123);
}
TEST(ConstantTests, LoadSecondConstant) {
    Assembler assembler;
    const Segment* intVal = assembler.AllocSegment(SegmentType::DATA,[](TBuffer* b) -> void {
        b->pushInt64(321);
    });
    const Segment* intVal2 = assembler.AllocSegment(SegmentType::DATA,[](TBuffer* b) -> void {
        b->pushInt64(123);
    });
    const auto call_test = assembler.CreateFunction("call_test", [intVal2](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->movReg_mem(X64Register::RAX, intVal2);

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    EXPECT_EQ(code->runFunction<int>(call_test->name), 123);
}
TEST(ConstantTests, LoadConstantAddress) {
    Assembler assembler;
    const Segment* intVal = assembler.AllocSegment(SegmentType::DATA,[](TBuffer* b) -> void {
        b->pushInt64(123);
    });
    const auto call_test = assembler.CreateFunction("call_test", [intVal](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->leaReg(X64Register::RAX, intVal);

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    EXPECT_EQ(*code->runFunction<int*>(call_test->name), 123);
}

TEST(BufferTests, BufferSaveLoadInt) {
    constexpr int i = 123;
    TBuffer buffer;
    buffer.pushInt32(i);
    buffer.resetHead();
    EXPECT_EQ(i, buffer.readInt32());
}
TEST(BufferTests, BufferSaveLoadIntBigEnd) {
    constexpr int i = 123;
    TBuffer buffer;
    buffer.bigEndian();
    buffer.pushInt32(i);
    buffer.resetHead();
    EXPECT_EQ(i, buffer.readInt32());
}
TEST(BufferTests, BufferSaveLoadFloat) {
    constexpr float i = 3.14;
    constexpr float d = 233.14;
    TBuffer buffer;
    buffer.pushFloat(i);
    buffer.pushDouble(d);
    buffer.resetHead();
    EXPECT_NEAR(i, buffer.readFloat(), .0001);
    EXPECT_NEAR(d, buffer.readDouble(), .0001);
}
TEST(BufferTests, BufferSaveLoadFloatBigEnd) {
    constexpr float i = 3.14;
    constexpr float d = 233.14;
    TBuffer buffer;
    buffer.bigEndian();
    buffer.pushFloat(i);
    buffer.pushDouble(d);
    buffer.resetHead();
    EXPECT_NEAR(i, buffer.readFloat(), .0001);
    EXPECT_NEAR(d, buffer.readDouble(), .0001);
}
TEST(BufferTests, BufferSaveLoadString) {
    const std::string s = "Hello world?";
    TBuffer buffer;
    buffer.pushString(s);
    buffer.resetHead();
    EXPECT_EQ(s.compare(buffer.readString()), 0);
}
TEST(BufferTests, BufferSaveLoadFromFile) {
    const std::string s = "#include <gtest/gtest.h>\n";
    TBuffer buffer("../tests/tests.cpp");
    const std::string s1 = buffer.readLine();
    EXPECT_STREQ(s.c_str(), s1.c_str());
}

TEST(CoroutineTests, TestSimpleCoroutine) {
    Assembler assembler;
    const Segment* intVal = assembler.AllocSegment(SegmentType::DATA,[](TBuffer* b) -> void {
        b->pushInt64(123);
    });
    LinkingInfo::Standard()->AllocForAssembler(assembler);
    LinkingInfo::Standard()->LinkAssembler(assembler);
    const auto coroutine_test = assembler.CreateFunction("coroutine_test", [intVal](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->movReg_mem(X64Register::RAX, intVal);

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    const auto coroutine = code->createCoroutine("coroutine_test");
    EXPECT_EQ(coroutine->runUntilComplete<int>(), 123);
}
TEST(CoroutineTests, TestMoreComplexCoroutine) {
    Assembler assembler;
    const Segment* intVal = assembler.AllocSegment(SegmentType::DATA, [](TBuffer* b) -> void {
        b->pushInt64(123);
    });
    LinkingInfo::Standard()->AllocForAssembler(assembler);
    LinkingInfo::Standard()->LinkAssembler(assembler);
    const auto coroutine_test = assembler.CreateFunction("coroutine_test", [intVal](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->movReg_mem(X64Register::RAX, intVal);

        buf->async_pause();

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    const auto coroutine = code->createCoroutine("coroutine_test");
    EXPECT_EQ(coroutine->runUntilComplete<int>(), 123);
}
TEST(CoroutineTests, TestCoroutineErrorRaise) {
    Assembler assembler;
    const Segment* intVal = assembler.AllocSegment(SegmentType::DATA, [](TBuffer* b) -> void {
        b->pushInt64(123);
    });
    LinkingInfo::Standard()->AllocForAssembler(assembler);
    LinkingInfo::Standard()->LinkAssembler(assembler);
    assembler.CreateFunction("coroutine_test", [intVal](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->movReg_mem(X64Register::RAX, intVal);

        buf->async_pause();

        buf->movReg(X64Register::RAX, 123);
        buf->async_raise();

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    const auto coroutine = code->createCoroutine("coroutine_test");
    coroutine->try_runUntilComplete();
    EXPECT_EQ(coroutine->error<int>(), 123);
}

TEST(AssemblerSerializationTests, SaveLoadAssembler) {
    Assembler assembler;
    assembler.CreateFunction("call_test", [](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->movReg(X64Register::RAX, 123);

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    EXPECT_EQ(code->runFunction<int>("call_test"), 123);
    TBuffer buffer;
    assembler.PushToBuffer(buffer);

    Assembler assembler1;
    buffer.resetHead();
    assembler1.PopFromBuffer(buffer);
    GeneratedCode* code1 = GeneratedCode::Create();
    assembler1.AssembleAll(code1);
    EXPECT_EQ(code1->runFunction<int>("call_test"), 123);
}
TEST(AssemblerScopeTests, SimpleAssemblerDeleteTest) {
    GeneratedCode* code = GeneratedCode::Create();
    {
        Assembler assembler;
        assembler.CreateFunction("call_test", [](CodeBuffer* buf) -> void {
            buf->prolog(0x20, {X64Register::RBX});

            buf->movReg(X64Register::RAX, 123);

            buf->epilog();
            buf->ret();
        });
        assembler.AssembleAll(code);
    }
    EXPECT_EQ(code->runFunction<int>("call_test"), 123);
}

TEST_F(LinkingTest, LinkingTest) {
    Assembler assembler;
    SetupLinking(assembler);
    const auto reg_test = assembler.CreateFunction("reg_test", [this, assembler](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX});

        buf->movReg(X64Register::RCX, 123);
        buf->callLinkedFunction(linkingInfo, "link_test");
        buf->movReg(X64Register::RAX, 0);

        buf->epilog();
        buf->ret();
    });
    GeneratedCode* code = GeneratedCode::Create();
    assembler.AssembleAll(code);
    EXPECT_EQ(code->runFunction<int>(reg_test->name), 0);
    EXPECT_EQ(CheckValue(), 123);
}