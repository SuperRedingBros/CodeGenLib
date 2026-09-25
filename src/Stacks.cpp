#include "../include/Stacks.h"

#include <memoryapi.h>

#define TStackSize 0xffff

TStack::TStack(void *address) :
    base(address), current(static_cast<uint8_t*>(address)) {
}

void TStack::free() {
    isFree = true;
    current = static_cast<uint8_t*>(base);
}

void TStackManager::CreateNewStack() {
    void* address = VirtualAlloc(nullptr, TStackSize, MEM_COMMIT, PAGE_READWRITE);
    stacks.push_back(new TStack(address));
}

TStackManager* TStackManager::GetInstance() {
    static TStackManager s_Instance;
    return &s_Instance;
}

TStack *TStackManager::GetFreeStack(size_t neededSize) {
    for (const auto stack : stacks) {
        if (stack->isFree) {
            stack->isFree = false;
            return stack;
        }
    }
    CreateNewStack();
    return stacks.back();
}
