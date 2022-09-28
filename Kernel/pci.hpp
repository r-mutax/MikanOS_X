#pragma once

#include <cstdint>
#include <array>

#include "error.hpp"

namespace pci
{
    /*
        memo
        PCIコンフィギュレーション空間にPCIデバイスの情報が載っている。
        そこにアクセスするにはコンフィギュレーション空間のどこを見たいか？を、
        CONFIG_ADDRESSレジスタに書き込み、CONFIG_DATAレジスタから読み取る。
        この２つのレジスタはIOアドレス空間に存在するので、それ専用の命令でのアクセスが必要。
        そのためasmfunc.asmにIoOut32()、IoIn32()命令を用意して、アクセスできるようにしている。
    */

    // CONFIG_ADDRESS レジスタのIOポートアドレス
    const uint16_t kConfigAddress = 0xcf8;

    // CONFIG_DATA　レジスタのIOポートアドレス
    const uint16_t kConfigData = 0xcfc;
    
    // PCIデバイスのクラスコード
    // Base ClassとSub Classがある。
    struct ClassCode {
        uint8_t base, sub, interface;

        // baseクラスの一致チェック
        bool Match(uint8_t b) { return b == base; }

        // baseとsubが等しいか？
        bool Match(uint8_t b, uint8_t s) { return Match(b) && s == sub; }

        // base, sub, インタフェースが等しいか？
        bool Match(uint8_t b, uint8_t s, uint8_t i){
            return Match(b, s) && i == interface;
        }
    };

    // デバイス操作用の基礎データ
    struct Device {
        uint8_t bus, device, function, header_type;
        ClassCode class_code;
    };

    // CONFIG_ADDRESSレジスタにアドレスを書き込む
    void WriteAddress(uint32_t address);

    // CONFIG_DATAにデータを書き込む
    void WriteData(uint32_t value);

    uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);
    uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);
    uint8_t  ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);
    ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);
    WithError<uint64_t> ReadBar(const Device& dev, unsigned int offset);

    inline uint16_t ReadVendorId(const Device& dev){
        return ReadVendorId(dev.bus, dev.device, dev.function);
    }

    uint32_t ReadConfReg(const Device& dev, uint8_t reg_addr);
    void WriteConfReg(const Device& dev, uint8_t reg_addr, uint32_t value);

    // バス番号レジスタを読取る
    uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);

    bool IsSingleFunctionDevice(uint8_t header_type);

    // PCIデバイスの一覧
    // 一つのPCIバスにPCIデバイスは最大32個接続できる
    inline std::array<Device, 32> devices;

    // devineの有効な要素の数
    inline int num_device;

    // PCIデバイスの全スキャン
    Error ScanAllBus();

    constexpr uint8_t CalcBarAddress(unsigned int bar_index) {
        return 0x10 + 4 * bar_index;
    }

    WithError<uint64_t> ReadBar(Device& device, unsigned int bar_index);

    union CapabilityHeader {
        uint32_t data;
        struct {
            uint32_t cap_id : 8;
            uint32_t next_ptr : 8;
            uint32_t cap : 16;
        }__attribute__((packed)) bits;
    } __attribute__((packed));

    const uint8_t kCapabilityMSI = 0x05;
    const uint8_t kCapabilityMSIX = 0x11;

    struct MSICapability {
        union {
            uint32_t data;
            struct {
                uint32_t cap_id : 8;
                uint32_t next_ptr : 8;
                uint32_t msi_enable : 1;
                uint32_t multi_msg_capable : 3;
                uint32_t multi_msg_enable : 3;
                uint32_t addr_64_capable : 1;
                uint32_t per_vector_mask_capable : 1;
                uint32_t : 7;
            } __attribute__((packed)) bits;
        } __attribute__((packed)) header;

        uint32_t msg_addr;
        uint32_t msg_upper_addr;
        uint32_t msg_data;
        uint32_t mask_bits;
        uint32_t pending_bits;
    } __attribute__((packed));

    // MSI または MSI-X 割り込みを設定する
    Error ConfigureMSI(const Device& dev, uint32_t msg_addr, uint32_t msg_data,
                        unsigned int num_vector_exponent);
    
    enum class MSITriggerMode {
        kEdge = 0,
        kLevel = 1
    };

    enum class MSIDeliveryMode {
        kFixed          = 0b000,
        kLowestPriority = 0b001,
        kSMI            = 0b010,
        kNMI            = 0b100,
        kINIT           = 0b101,
        kExtINT         = 0b111
    };

    Error ConfigureMSIFixedDestination(
        const Device& dev, uint8_t apic_id,
        MSITriggerMode trigger_mode, MSIDeliveryMode delivery_mode,
        uint8_t vector, unsigned int num_vector_exponent
    );
}
