#ifndef CODE_GEN_TESTS_ASYNC_H
#define CODE_GEN_TESTS_ASYNC_H
#include <cstdint>
#include <expected>

#include "Assembler.h"
#include "Stacks.h"

class GeneratedCode;

enum class TCoroutineCode : uint8_t {
    IN_PROGRESS,
    FINISHED,
    ERROR,
    SLEEP
};

#define CoroutineStackSize 0xffff

struct TCoroutine;

class TCoroutineError : public std::runtime_error {
public:
    explicit TCoroutineError(const char *string, const TCoroutine* offendingCoroutine);
    const TCoroutine* offendingCoroutine;
};

struct TCoroutine {
    /**
     * Will block the thread if async_sleep is called
     *
     * @tparam T type of coroutine result
     * @return Returns value of type T, the return value of the coroutine
     */
    template<typename T>
    [[nodiscard]] T runUntilComplete() {
        runUntilComplete();
        return static_cast<T>(data);
    }

    void runUntilComplete();

    /**
     * Will block the thread if async_sleep is called
     * If an error occurs, stops execution
     */
    void try_runUntilComplete();

    void next();

    void try_next();

    template<typename T>
    [[nodiscard]] std::optional<T> result() const {
        if (code != TCoroutineCode::FINISHED) return std::nullopt;
        return static_cast<T>(data);
    }

    template<typename E>
    [[nodiscard]] std::optional<E> error() const {
        if (code != TCoroutineCode::ERROR) return std::nullopt;
        return static_cast<E>(data);
    }

    /**
     * If the coroutine is still in progress, returns unexpected result
     */
    template<typename T, typename E>
    [[nodiscard]] std::expected<T, E> expectFinished() const {
        if (code == TCoroutineCode::FINISHED)
            return static_cast<T>(data);
        if (code == TCoroutineCode::ERROR)
            return static_cast<E>(data);
        return std::unexpect;
    }

    [[nodiscard]] TCoroutineCode coroutineCode() const {
        return code;
    }

    [[nodiscard]] static TCoroutine* Create(void* address);

    static void(*getAsyncRaise())(TCoroutine*);
    static void(*getAsyncRet())(TCoroutine*);
    static void(*getAsyncPause())(TCoroutine*);
    static void(*getAsyncSleep())(TCoroutine*);

    TCoroutine(void* address, uint8_t* stackBase, size_t stackSize = CoroutineStackSize, TStack* stackPtr = nullptr);
private:
    static void(*getAsyncContinue())(TCoroutine*);
    static GeneratedCode* getAsyncFunctions(Assembler *&assembler_out);

    void sleepCoroutine();

    void* address;
    TStack* stackPtr;
    uint64_t data = 0;
    TCoroutineCode code = TCoroutineCode::IN_PROGRESS;
};


#endif //CODE_GEN_TESTS_ASYNC_H
