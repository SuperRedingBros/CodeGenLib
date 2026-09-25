#include "../include/LinkingInfo.h"

#include <iostream>

#include "../include/Assembler.h"
#include "../include/Async.h"

static void linkedPrint(const char* cstring) {
    std::cout << cstring << std::endl;
}

static void linkedDebug() {
    printf("Error occurred!");
}

LinkingInfo* LinkingInfo::Standard() {
    static auto linkingInfo = LinkingInfo();
    linkingInfo.AddLinkedFunction("print", linkedPrint);
    linkingInfo.AddLinkedFunction("debug", linkedDebug);
    linkingInfo.AddLinkedFunction("on_except", linkedDebug);
    linkingInfo.AddLinkedFunction("malloc", malloc);
    linkingInfo.AddLinkedFunction("free", free);
    linkingInfo.AddLinkedFunction("async_pause", TCoroutine::getAsyncPause());
    linkingInfo.AddLinkedFunction("async_raise", TCoroutine::getAsyncRaise());
    linkingInfo.AddLinkedFunction("async_sleep", TCoroutine::getAsyncSleep());
    linkingInfo.AddLinkedFunction("async_ret", TCoroutine::getAsyncRet());
    return &linkingInfo;
}

const CodeLabel * LinkingInfo::GetLinkedFunction(const Assembler &assembler, const std::string &func_name) const {
    if (!entries.contains(func_name)) {
        return nullptr;
    }
    return assembler.GetLabel("Linked_"+func_name);
}
const CodeLabel * LinkingInfo::GetLinkedFunction(const Assembler* assembler, const std::string &func_name) const {
    if (!entries.contains(func_name)) {
        return nullptr;
    }
    return assembler->GetLabel("Linked_"+func_name);
}

void LinkingInfo::LinkAssembler(const Assembler& assembler) const {
    const auto seg = assembler.GetSegment("LinkingInfo");
    seg->buffer.clear();
    seg->buffer.resetHead();
    for (int i = 0; i < entry_count; ++i) {
        for (auto &val: entries | std::views::values) {
            if (val.entry_number == i) {
                seg->buffer.pushInt64(reinterpret_cast<uint64_t>(val.func_ptr));
                break;
            }
        }
    }
}

void LinkingInfo::AllocForAssembler(Assembler& assembler) const {
    assembler.AllocSegment(SegmentType::DATA,"LinkingInfo", [this](TBuffer* b) -> void {
        b->resize(entries.size() * 8);
    });
    for (auto &val: entries | std::views::values) {
        const auto label = assembler.ReserveLabel("Linked_"+val.functionName);
        label->offsetInFunc = (val.entry_number * 8);
    }
}
