#ifndef CODE_GEN_TESTS_LINKINGINFO_H
#define CODE_GEN_TESTS_LINKINGINFO_H
#include <cstdint>
#include <string>
#include <utility>

#include "Assembler.h"

class LinkingEntry {
    const std::string functionName;
    const void* func_ptr;
    const uint16_t entry_number;
    friend class LinkingInfo;
    explicit LinkingEntry(std::string functionName, const void* func_ptr, const uint16_t entryNumber)
        : functionName {std::move(functionName)}, func_ptr  {func_ptr}, entry_number {entryNumber} {}
};

class LinkingInfo {
    std::string name {};
    uint16_t version {};
    uint16_t sub_version {};

    std::unordered_map<std::string, LinkingEntry> entries {};
    uint16_t entry_count {};
public:
    static LinkingInfo* Standard();

    template<typename R, typename ...Args>
    void AddLinkedFunction(const std::string& func_name, R(func)(Args...)) {
        entries.emplace(func_name, LinkingEntry {func_name, reinterpret_cast<void*>(func), entry_count++});
    }

    const CodeLabel *GetLinkedFunction(const Assembler &assembler, const std::string &func_name) const;

    const CodeLabel *GetLinkedFunction(const Assembler *assembler, const std::string &func_name) const;

    const CodeLabel * GetPrint(const Assembler& assembler) const {
        return GetLinkedFunction(assembler, "print");
    }

    const CodeLabel * GetMalloc(const Assembler& assembler) const {
        return GetLinkedFunction(assembler, "malloc");
    }

    const CodeLabel * GetFree(const Assembler& assembler) const {
        return GetLinkedFunction(assembler, "free");
    }

    void LinkAssembler(const Assembler& assembler) const;

    void AllocForAssembler(Assembler &assembler) const;
};

#endif //CODE_GEN_TESTS_LINKINGINFO_H
