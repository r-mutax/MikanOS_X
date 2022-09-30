#pragma once

#include <array>
#include <cstdint>

#include "x86_descriptor.hpp"

union InterruptDescriptorAttribute {
    uint16_t data;
    struct {
        uint16_t interrupt_stack_table : 3;
        uint16_t : 5;
        DescriptorType type: 4;                     // 記述子のタイプ
        uint16_t : 1;
        uint16_t descriptor_privilege_level : 2;    // DPL 割り込みハンドラの実行レベル
        uint16_t present : 1;                       // 記述子の有効/無効
    } __attribute__((packed)) bits;
} __attribute__((packed));

// __attribute__((packed))
// -> 構造体にパディングを詰めない。

struct InterruptDescriptor {
    uint16_t offset_low;                    // ハンドラのアドレス
    uint16_t segment_selector;              // セグメント
    InterruptDescriptorAttribute attr;      // アトリビュート
    uint16_t offset_middle;                 // ハンドラのアドレス
    uint32_t offset_high;                   // ハンドラのアドレス
    uint32_t reserved;
} __attribute__ ((packed));

extern std::array<InterruptDescriptor, 256> idt;

constexpr InterruptDescriptorAttribute MakeIDTAttr (
    DescriptorType type,
    uint8_t descriptor_privilege_level,
    bool present = true,
    uint8_t interrupt_stack_table = 0) {
    
    InterruptDescriptorAttribute attr{};
    attr.bits.interrupt_stack_table = interrupt_stack_table;
    attr.bits.type = type;
    attr.bits.descriptor_privilege_level = descriptor_privilege_level;
    attr.bits.present = present;
    return attr;
}

void SetIDTEntry(InterruptDescriptor& desc,
                InterruptDescriptorAttribute attr,
                uint64_t offset,
                uint16_t segment_selector);

class InterruptVector {
    public:
        enum Number {
            kXHCI = 0x40,
        };
};

struct InterruptFrame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

void NotifyEndOfInterrupt();
