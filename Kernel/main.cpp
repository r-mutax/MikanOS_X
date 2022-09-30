#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "mouse.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "logger.hpp"
#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"
#include "interrupt.hpp"
#include "memory_map.hpp"
#include "asmfunc.h"
#include "queue.hpp"

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};


// void* operator new(size_t size, void* buf) {
//   return buf;
// }

void operator delete(void* obj) noexcept {
}

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

char console_buf[sizeof(Console)];
Console* console;

int printk(const char* fmt, ...){
    va_list ap;
    int result;
    char s[1024];

    va_start(ap, fmt);
    result = vsprintf(s, fmt, ap);
    va_end(ap);

    console->PutString(s);
    return result;
}

char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor* mouse_cursor;

void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
    mouse_cursor->MoveRelative({displacement_x, displacement_y});
}

void SwitchEhci2Xhci(const pci::Device& xhc_dev) {
    bool intel_ehc_exist = false;
    for(int i = 0; i < pci::num_device; ++i) {
        if(pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u)
            && 0x8086 == pci::ReadVendorId(pci::devices[i])){
            intel_ehc_exist = true;
            break;
        }
    }

    if(!intel_ehc_exist) return;

    uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, 0xdc);    // USB3PRM
    pci::WriteConfReg(xhc_dev, 0xd8, superspeed_ports);             // USB3_PSSEN
    uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, 0xd4);     // XUSB2PRM
    pci::WriteConfReg(xhc_dev, 0xd0, ehci2xhci_ports);              // XUSB2PR
    Log(kDebug, "SwitchEhci2Xhci: SS = %02, xHCI = %02x\n",
        superspeed_ports, ehci2xhci_ports);
}

usb::xhci::Controller* xhc;

struct Message {
    enum Type {
        kInterruptXHCI,
    } type;
};

ArrayQueue<Message>* main_queue;

__attribute__((interrupt))
void IntHandlerXHCI(InterruptFrame* frame) {

    main_queue->Push(Message{Message::kInterruptXHCI});
    NotifyEndOfInterrupt();
}

extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config, const MemoryMap& memory_map){

    switch(frame_buffer_config.pixel_format){
        case kPixelRGBResv8BitPerColor:
            pixel_writer = new(pixel_writer_buf)
            RGBResv8BitPerColorPixelWriter{frame_buffer_config};
            break;
        case kPixelBGRResv8BitPerColor:
            pixel_writer = new(pixel_writer_buf)
            BGRResv8BitPerColorPixelWriter{frame_buffer_config};
            break;
    }

    const int kFrameWidth = frame_buffer_config.horizontal_resolution;
    const int kFrameHeight = frame_buffer_config.vertical_resolution;

    // 背景の描画
    FillRectangle(*pixel_writer,
                    {0, 0},
                    {kFrameWidth, kFrameHeight - 50},
                    kDesktopBGColor);

    // タスクバーの描画
    FillRectangle(*pixel_writer,
                    {0, kFrameHeight - 50},
                    {kFrameWidth, 50},
                    {1, 8, 17});

    // スタートボタンのあたり？
    FillRectangle(*pixel_writer,
                    {0, kFrameHeight - 50},
                    {kFrameWidth / 5, 50},
                    {80, 80, 80});

    // スタートボタンの枠？
    DrawRectangle(*pixel_writer,
                    {10, kFrameHeight - 40},
                    {30, 30},
                    {160, 160, 160});

    // マウスカーソルの描画

    console = new(console_buf) Console{
      *pixel_writer, kDesktopFGColor, kDesktopBGColor
      };
    printk("Welcome to MikanOS_X\n");
    SetLogLevel(kWarn);

    const std::array available_memory_types {
        MemoryType::kEfiBootServicesCode,
        MemoryType::kEfiBootServicesData,
        MemoryType::kEfiConventionalMemory,
    };

    printk("memory_map: %p\n", &memory_map);
    for(uintptr_t iter = reinterpret_cast<uintptr_t>(memory_map.buffer);
        iter < reinterpret_cast<uintptr_t>(memory_map.buffer) + memory_map.map_size;
        iter += memory_map.descriptor_size){
        auto desc = reinterpret_cast<MemoryDescripter*>(iter);
        for(int i = 0; i < available_memory_types.size(); ++i){
            if(desc->type == available_memory_types[i] ){
                printk("type = %u, phys = %08lx - %08lx, pages = %lu, attr = %08lx\n",
                    desc->type,
                    desc->physical_start,
                    desc->physical_start + desc->number_of_pages * 4096 - 1,
                    desc->number_of_pages,
                    desc->attribute);
            }
        }
    }

    mouse_cursor = new(mouse_cursor) MouseCursor {
        pixel_writer, kDesktopBGColor, {300, 200}
    };

    std::array<Message, 32> main_queue_data;
    ArrayQueue<Message> main_queue{main_queue_data};
    ::main_queue = &main_queue;

    // PCIデバイスを列挙する
    auto err = pci::ScanAllBus();
    Log(kDebug, "ScanAllBus: %s\n", err.Name());

    for(int i = 0; i < pci::num_device; ++i){
        const auto& dev = pci::devices[i];
        auto vendor_id = pci::ReadVendorId(dev);
        auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);

        Log(kDebug, "%d.%d.%d: vend %04x, classcode [bs]%02x [sb]%02x [if]%02x, head %02x\n",
                dev.bus, dev.device, dev.function,
                vendor_id, class_code.base, class_code.sub, class_code.interface, dev.header_type);
    }

    // xhcのデバイスを探す
    pci::Device* xhc_dev = nullptr;
    for(int i = 0; i < pci::num_device; ++i){
        if(pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)){
            xhc_dev = &pci::devices[i];

            if(0x8086 == pci::ReadVendorId(*xhc_dev)) {
                Log(kInfo, "xHC device found\n");
                break;
            }
        }
    }

    if(xhc_dev){
        Log(kInfo, "xHC has been found: %d.%d.%d\n",
         xhc_dev->bus, xhc_dev->device, xhc_dev->function);
    }

    // Interrupt Daescripter TableにIntHandlerXHCI()を登録する
    const uint16_t cs = GetCS();
    SetIDTEntry(idt[InterruptVector::kXHCI],MakeIDTAttr(DescriptorType::kInterruptGate, 0),
                reinterpret_cast<uint64_t>(IntHandlerXHCI), cs);

    LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

    // BDP(Bootstrap Processor)の番号を読み取る
    const uint8_t bsp_local_apic_id =
        *reinterpret_cast<const uint32_t*>(0xfee00020) >> 24;

    pci::ConfigureMSIFixedDestination(
        *xhc_dev, bsp_local_apic_id,
        pci::MSITriggerMode::kLevel, pci::MSIDeliveryMode::kFixed,
        InterruptVector::kXHCI, 0);

    // xhcのデバイスのMMIO(メモリマップドIO)のアドレスを探す
    const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
    Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
    const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
    Log(kDebug, "xHC mmio_base = %08lx\n", xhc_mmio_base);

    // xhcのMMIOのアドレスから、コントローラクラスを作る
    usb::xhci::Controller xhc{xhc_mmio_base};

    if (0x8086 == pci::ReadVendorId(*xhc_dev)){
        SwitchEhci2Xhci(*xhc_dev);
    }
    {
        auto err = xhc.Initialize();
        Log(kDebug, "xhc.Initialize: %s\n", err.Name());
    }
    Log(kInfo, "xHC starting\n");
    xhc.Run();

    ::xhc = &xhc;
    __asm__("sti");

    usb::HIDMouseDriver::default_observer = MouseObserver;

    for(int i = 1; i <= xhc.MaxPorts(); ++i) {
        auto port = xhc.PortAt(i);
        Log(kDebug, "Port %d: IsConnected = %d\n", i, port.IsConnected());

        if(port.IsConnected()) {
            if(auto err = ConfigurePort(xhc, port)) {
                Log(kError, "failed to configure port: %s at %s:%d\n",
                    err.Name(), err.File(), err.Line());
                continue;
            }
        }
    }

    // ==============================================================================
    // 割り込みで受け取ったメッセージを処理する
    while (true){

        // 今からメッセージキューからメッセージを取り出す。
        // その間に割り込みが起きてメッセージが追加されると困るので、
        // cli命令を実行して割り込み許可フラグをゼロにする。
        __asm__("cli");
        if(main_queue.Count() == 0){
            // メッセージキューがからの場合は、hlt命令実行して省電力モードにする
            // 省電力モードでは、割り込みが起きるまでCPUが止まる？
            // その際に、次の割り込みが起こらなくなると困るので、
            // sti命令で割り込み許可フラグを立てておく。
            __asm__("sti\n\thlt");
            continue;
        }

        Message msg = main_queue.Front();
        main_queue.Pop();
        __asm__("sti");
        // メッセージの取得が完了したらsti命令で割り込み許可フラグを戻す。

        switch(msg.type){
            case Message::kInterruptXHCI:
                while(xhc.PrimaryEventRing()->HasFront()){
                    if(auto err = ProcessEvent(xhc)){
                        Log(kError, "Error while ProcessEvent: %s at %s:%d\n",
                        err.Name(), err.File(), err.Line());
                    }
                }
                break;            
            default:
                Log(kError, "Unknown message type: %d\n", msg.type);
                break;
        }


    }

    while(1) __asm__("hlt");
}