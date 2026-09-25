#ifndef CODE_GEN_TESTS_STACKS_H
#define CODE_GEN_TESTS_STACKS_H
#include <cstdint>
#include <vector>

class TStack {
public:
    void* base{};
    uint8_t* current{};
    uint8_t isSystem: 1 = 0;
    uint8_t isGuarded: 1 = 1;
    uint8_t isFree: 1 = 1;
    explicit TStack(void* address);

    void free();
};

class TStackManager {
    std::vector<TStack*> stacks;
    void CreateNewStack();
public:
    static TStackManager* GetInstance();

    [[nodiscard]] TStack *GetFreeStack(size_t neededSize);
};


#endif //CODE_GEN_TESTS_STACKS_H
