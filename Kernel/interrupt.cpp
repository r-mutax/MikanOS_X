#include "interrupt.hpp"

std::array<InterruptDescriptor, 256> idt;

// 割り込みベクタの設定？
void SetIDTEntry(InterruptDescriptor& desc,
                InterruptDescriptorAttribute attr,
                uint64_t offset,
                uint16_t segment_selector){
    desc.attr = attr;
    desc.offset_low = offset & 0xffffu;
    desc.offset_middle = (offset >> 16) & 0xffffu;
    desc.offset_high = offset >> 32;
    desc.segment_selector = segment_selector;
}

// 割り込み処理の完了をCPUに伝える
void NotifyEndOfInterrupt(){
    volatile auto end_of_interrupt = reinterpret_cast<uint32_t*>(0xfee000b0);
    *end_of_interrupt = 0;
}