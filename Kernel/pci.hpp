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
}
