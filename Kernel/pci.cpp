#include "pci.hpp"
#include "asmfunc.h"

namespace
{
    using namespace pci;

    uint32_t MakeAddress(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg_addr){
        auto shl = [](uint32_t x, unsigned int bits){
            return x << bits;
        };

        return shl(1, 31) | shl(bus, 16) | shl(device, 11) | shl(function, 8) | (reg_addr & 0xfcu);
    }

    Error AddDevice(const Device& device) {
        if(num_device == devices.size()) {
            return MAKE_ERROR(Error::kFull);
        }

        devices[num_device] = device;
        ++num_device;
        return MAKE_ERROR(Error::kSuccess);
    }

    Error ScanBus(uint8_t bus);
    
    Error ScanFunction(uint8_t bus, uint8_t device, uint8_t function) {
        auto header_type = ReadHeaderType(bus, device, function);
        auto class_code = ReadClassCode(bus, device, function);
        Device dev{bus, device, function, header_type, class_code};
        if(auto err = AddDevice(dev)){
            // 32個以上のデバイスを登録している場合、デバイスが多すぎる
            // ERRORを返してデバイスの探索を中断する
            return err;
        }

        if(class_code.Match(0x06u, 0x04u)) {
            // standard PCI-PCI bridge

            // 指定したPCIデバイスがPCI-PCIブリッジの場合は、サブブリッジのスキャンを行う？
            auto bus_numbers = ReadBusNumbers(bus, device, function);
            uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
            return ScanBus(secondary_bus);
        }

        return MAKE_ERROR(Error::kSuccess);        
    }

    Error ScanDevice(uint8_t bus, uint8_t device) {
        if(auto err = ScanFunction(bus, device, 0)){
            return err;
        }

        if(IsSingleFunctionDevice(ReadHeaderType(bus, device, 0))) {
            // 読み取ったデバイスがSingleFunctionならそれで終わり
            return MAKE_ERROR(Error::kSuccess);
        }

        for(uint8_t function = 1; function < 8; ++function) {
            if(ReadVendorId(bus, device, function) == 0xffffu) {
                continue;
            }

            if(auto err = ScanFunction(bus, device, function)){
                return err;
            }
        }
        return MAKE_ERROR(Error::kSuccess);
    }

    Error ScanBus(uint8_t bus) {
        for(uint8_t device = 0; device < 32; ++device){
            if(ReadVendorId(bus, device, 0) == 0xffffu) {
                continue;
            }
            if(auto err = ScanDevice(bus, device)) {
                return err;
            }
        }
        return MAKE_ERROR(Error::kSuccess);
    }

}   

namespace pci {

    void WriteAddress(uint32_t address){
        IoOut32(pci::kConfigAddress, address);
    }

    void WriteData(uint32_t value) {
        IoOut32(pci::kConfigData, value);
    }

    uint32_t ReadData() {
        return IoIn32(pci::kConfigData);
    }

    uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function) {
        WriteAddress(MakeAddress(bus, device, function, 0x00));
        return ReadData() & 0xffffu;
    }

    uint16_t ReadDeviceID(uint8_t bus, uint8_t device, uint8_t function) {
        WriteAddress(MakeAddress(bus, device, function, 0x00));
        return ReadData() >> 16;
    }

    uint8_t  ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
        WriteAddress(MakeAddress(bus, device, function, 0x0c));
        return (ReadData() >> 16) & 0xff;
    }

    ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function) {

        WriteAddress(MakeAddress(bus, device, function, 0x08));
        auto reg = ReadData();

        ClassCode cc;
        cc.base = (reg >> 24);
        cc.sub = (reg >> 16) & 0xffu;
        cc.interface = (reg >> 8) & 0xffu;

        return cc;
    }

    uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function) {
        WriteAddress(MakeAddress(bus, device, function, 0x18));
        return ReadData();
    }

    bool IsSingleFunctionDevice(uint8_t header_type){
        return (header_type & 0x80u) == 0;
    }

    Error ScanAllBus() {
        num_device = 0;
        auto header_type = ReadHeaderType(0, 0, 0);
        if(IsSingleFunctionDevice(header_type)){
            return ScanBus(0);
        }

        for (uint8_t function = 1; function < 8; ++function){
            if(ReadVendorId(0, 0, function) == 0xffffu){
                // ベンダID = 0xffffuのファンクションは、無効なファンクション
                continue;
            }
            if(auto err = ScanBus(function)) {
                return err;
            }
        }
        return MAKE_ERROR(Error::kSuccess);
    }
}