#include "../include/Async.h"

#include <iostream>
#include <thread>
#include <cstring>

#include "../include/Assembler.h"
#include "../include/GeneratedCode.h"
#include "../include/Stacks.h"

TCoroutineError::TCoroutineError(const char *string, const TCoroutine *offendingCoroutine)
        : runtime_error(string), offendingCoroutine(offendingCoroutine) {
}

TCoroutine::TCoroutine(void *address, uint8_t *stackBase, const size_t stackSize, TStack* stackPtr) :
        address(address),
        stackPtr(stackPtr) {
    memset(stackBase, 0, stackSize);
    stackPtr->current = stackPtr->current + stackSize;
    const auto top = reinterpret_cast<void**>(stackPtr->current);
    top[-1] = reinterpret_cast<void*>(this);
    stackPtr->current -= 8;
    top[-2] = reinterpret_cast<void*>(getAsyncRet());
    stackPtr->current -= 8;
}

void TCoroutine::runUntilComplete()  {
    try_runUntilComplete();
    if (code == TCoroutineCode::ERROR) {
        stackPtr->free();
        throw TCoroutineError("Coroutine had an error", this);
    }
}

void TCoroutine::try_runUntilComplete()  {
    if (code == TCoroutineCode::SLEEP) sleepCoroutine();
    if (code != TCoroutineCode::IN_PROGRESS) {
        throw TCoroutineError("Coroutine was completed or had an error", this);
    }
    while (true) {
        try_next();
        if (code == TCoroutineCode::FINISHED || code == TCoroutineCode::ERROR) {
            return;
        }
        if (code == TCoroutineCode::SLEEP) sleepCoroutine();
    }
}

void TCoroutine::next() {
    try_next();
    if (code == TCoroutineCode::ERROR) {
        stackPtr->free();
        throw TCoroutineError("Coroutine had an error", this);
    }
}

void TCoroutine::try_next() {
    getAsyncContinue()(this);
    if (code == TCoroutineCode::FINISHED || code == TCoroutineCode::ERROR) {
        stackPtr->free();
    }
}

TCoroutine * TCoroutine::Create(void *address)  {
    return new TCoroutine(address, static_cast<uint8_t*>(malloc(CoroutineStackSize)), CoroutineStackSize,
        TStackManager::GetInstance()->GetFreeStack(CoroutineStackSize));
}

void TCoroutine::sleepCoroutine() {
    std::this_thread::sleep_for(std::chrono::milliseconds(data));
    code = TCoroutineCode::IN_PROGRESS;
}

// Statics:

GeneratedCode * TCoroutine::getAsyncFunctions(Assembler*& assembler_out) {
    static const auto assembler = new Assembler;
    static GeneratedCode* code = nullptr;
    if (code != nullptr) {
        assembler_out = assembler;
        return code;
    }
    assembler->CreateFunction("async_raise", [](CodeBuffer* buf) -> void {
        //Copy return value to data field of coroutine
        buf->movReg({
            .displacement = offsetof(TCoroutine, data),
            .base = X64Register::RCX
        }, X64Register::RAX, RegSize::X64);

        // Set exit code to finished
        buf->movReg(X64Register::RDX, static_cast<uint8_t>(TCoroutineCode::ERROR));
        buf->movReg({
            .displacement = offsetof(TCoroutine, code),
            .base = X64Register::RCX
        }, X64Register::RDX);

        //Jump to next address
        buf->movReg(X64Register::RAX, {
            .displacement = offsetof(TCoroutine, address),
            .base = X64Register::RCX
        }, RegSize::X64);
        buf->jumpToReg(X64Register::RAX);
    });
    assembler->CreateFunction("async_ret", [](CodeBuffer* buf) -> void {
        buf->popReg(X64Register::RCX);
        //Copy return value to data field of coroutine
        buf->movReg({
            .displacement = offsetof(TCoroutine, data),
            .base = X64Register::RCX
        }, X64Register::RAX, RegSize::X64);

        // Set exit code to finished
        buf->movReg(X64Register::RDX, static_cast<uint8_t>(TCoroutineCode::FINISHED));
        buf->movReg({
            .displacement = offsetof(TCoroutine, code),
            .base = X64Register::RCX
        }, X64Register::RDX);

        //Jump to next address
        buf->movReg(X64Register::RAX, {
            .displacement = offsetof(TCoroutine, address),
            .base = X64Register::RCX
        }, RegSize::X64);
        buf->jumpToReg(X64Register::RAX);
    });
    assembler->CreateFunction("async_pause", [](CodeBuffer* buf) -> void {
        buf->prolog(0x20, ALL_X64Registers);

        const auto label = buf->ReserveLabel("pause_next");

        buf->leaReg(X64Register::RDX, label);
        buf->movReg(X64Register::RAX, {
            .displacement = offsetof(TCoroutine, address),
            .base = X64Register::RCX
        }, RegSize::X64);
        buf->movReg({
            .displacement = offsetof(TCoroutine, address),
            .base = X64Register::RCX
        }, X64Register::RDX, RegSize::X64);
        buf->jumpToReg(X64Register::RAX);

        buf->markLabel(label);

        buf->epilog();
        buf->ret();
    });
    assembler->CreateFunction("async_sleep", [](CodeBuffer* buf) -> void {
        buf->prolog(0x20, ALL_X64Registers);
        //Copy sleep time to data field of coroutine
        buf->movReg({
            .displacement = offsetof(TCoroutine, data),
            .base = X64Register::RCX
        }, X64Register::RDX, RegSize::X64);

        //Put coroutine to sleep
        buf->movReg(X64Register::RDX, static_cast<uint8_t>(TCoroutineCode::SLEEP));
        buf->movReg({
            .displacement = offsetof(TCoroutine, code),
            .base = X64Register::RCX
        }, X64Register::RDX);

        const auto label = buf->ReserveLabel("pause_next");

        buf->leaReg(X64Register::RDX, label);
        buf->movReg(X64Register::RAX, {
            .displacement = offsetof(TCoroutine, address),
            .base = X64Register::RCX
        }, RegSize::X64);
        buf->movReg({
            .displacement = offsetof(TCoroutine, address),
            .base = X64Register::RCX
        }, X64Register::RDX, RegSize::X64);
        buf->jumpToReg(X64Register::RAX);

        buf->markLabel(label);

        buf->epilog();
        buf->ret();
    });
    assembler->CreateFunction("async_next", [](CodeBuffer* buf) -> void {
        buf->prolog(0x20, {X64Register::RBX, X64Register::R8});

        buf->movReg(X64Register::RBX, X64Register::RSP);

        buf->movReg(X64Register::R8, {
            .displacement = offsetof(TCoroutine, stackPtr),
            .base = X64Register::RCX
        }, RegSize::X64);
        buf->movReg(X64Register::RSP, {
            .displacement = offsetof(TStack, current),
            .base = X64Register::R8
        }, RegSize::X64);

        buf->movReg(X64Register::RAX, {
            .displacement = offsetof(TCoroutine, address),
            .base = X64Register::RCX
        }, RegSize::X64);
        const auto label = buf->ReserveLabel("continue_next");
        buf->leaReg(X64Register::RDX, label);
        buf->movReg({
            .displacement = offsetof(TCoroutine, address),
            .base = X64Register::RCX
        }, X64Register::RDX, RegSize::X64);
        buf->jumpToReg(X64Register::RAX);

        buf->markLabel(label);

        buf->movReg(X64Register::R8,{
            .displacement = offsetof(TCoroutine, stackPtr),
            .base = X64Register::RCX
        }, RegSize::X64);
        buf->movReg({
            .displacement = offsetof(TStack, current),
            .base = X64Register::R8
        }, X64Register::RSP, RegSize::X64);

        buf->movReg(X64Register::RSP, X64Register::RBX);

        buf->epilog();
        buf->ret();
    });
    code = GeneratedCode::Create();
    assembler->AssembleAll(code);
    assembler_out = assembler;
    return code;
}

void(*TCoroutine::getAsyncRaise())(TCoroutine*) {
    static void(*async_raise)(TCoroutine*) = nullptr;
    if (async_raise != nullptr) return async_raise;
    Assembler* assembler;
    const auto code = getAsyncFunctions(assembler);
    async_raise = code->getFunction<void, TCoroutine*>("async_raise");
    return async_raise;
}

void(*TCoroutine::getAsyncRet())(TCoroutine*) {
    static void(*async_ret)(TCoroutine*) = nullptr;
    if (async_ret != nullptr) return async_ret;
    Assembler* assembler;
    const auto code = getAsyncFunctions(assembler);
    async_ret = code->getFunction<void, TCoroutine*>("async_ret");
    return async_ret;
}

void(*TCoroutine::getAsyncPause())(TCoroutine*) {
    static void(*async_pause)(TCoroutine*) = nullptr;
    if (async_pause != nullptr) return async_pause;
    Assembler* assembler;
    const auto code = getAsyncFunctions(assembler);
    async_pause = code->getFunction<void, TCoroutine*>("async_pause");
    return async_pause;
}

void(*TCoroutine::getAsyncSleep())(TCoroutine*) {
    static void(*async_sleep)(TCoroutine*) = nullptr;
    if (async_sleep != nullptr) return async_sleep;
    Assembler* assembler;
    const auto code = getAsyncFunctions(assembler);
    async_sleep = code->getFunction<void, TCoroutine*>("async_sleep");
    return async_sleep;
}

void(*TCoroutine::getAsyncContinue())(TCoroutine*) {
    static void(*async_next)(TCoroutine*) = nullptr;
    if (async_next != nullptr) return async_next;
    Assembler* assembler;
    const auto code = getAsyncFunctions(assembler);
    async_next = code->getFunction<void, TCoroutine*>("async_next");
    return async_next;
}

