#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/BlockIo.h>
#include <Guid/FileInfo.h>
#include "frame_buffer_config.hpp"
#include "memory_map.hpp"
#include "elf.hpp"

// local function
EFI_STATUS GetMemoryMap(struct MemoryMap* map);
const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type);
EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file);
EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root);
EFI_STATUS OpenGOP(EFI_HANDLE image_handle, EFI_GRAPHICS_OUTPUT_PROTOCOL** gop);
void Halt();
const CHAR16* GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt);


EFI_STATUS GetMemoryMap(struct MemoryMap* map){
    if(map->buffer == NULL){
        return EFI_BUFFER_TOO_SMALL;
    }

    map->map_size = map->buffer_size;
    return gBS->GetMemoryMap(
        &map->map_size,
        (EFI_MEMORY_DESCRIPTOR*)map->buffer,
        &map->map_key,
        &map->descriptor_size,
        &map->descriptor_version);
}

const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type){
    switch(type){
        case EfiReservedMemoryType: return L"EfiReservedMemoryType";
        case EfiLoaderCode: return L"EfiLoaderCode";
        case EfiLoaderData: return L"EfiLoaderData";
        case EfiBootServicesCode: return L"EfiBootServicesCode";
        case EfiBootServicesData: return L"EfiBootServicesData";
        case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
        case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
        case EfiConventionalMemory: return L"EfiConventionalMemory";
        case EfiUnusableMemory: return L"EfiUnusableMemory";
        case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
        case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
        case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
        case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
        case EfiPalCode: return L"EfiPalCode";
        case EfiPersistentMemory: return L"EfiPersistentMemory";
        case EfiMaxMemoryType: return L"EfiMaxMemoryType";
        default: return L"InvalidMemoryType";
    }
}

EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file){
    EFI_STATUS status;
    CHAR8 buf[256];
    UINTN len;

    CHAR8* header = 
        "Index, Type, Type(name), PhisicalStart, NumberOfPages, Attribute\n";
    len = AsciiStrLen(header);
    status = file->Write(file, &len, header);
    if (EFI_ERROR(status)) {
        return status;
    }

    Print(L"map->buffer = %08lx, map->map_size = %08lx\n",
        map->buffer, map->map_size);
    
    EFI_PHYSICAL_ADDRESS iter;
    int i;
    for(iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
        iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
        iter += map->descriptor_size, i++){
        
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;
        len = AsciiSPrint(
            buf, sizeof(buf),
            "%u, %x, %-ls, %08lx, %lx, %lx\n",
            i, desc->Type, GetMemoryTypeUnicode(desc->Type),
            desc->PhysicalStart, desc->NumberOfPages,
            desc->Attribute & 0xffffflu);
        status = file->Write(file, &len, buf);
        if (EFI_ERROR(status)){
            return status;
        }
    }

    return EFI_SUCCESS;
}

EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root){
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

    status = gBS->OpenProtocol(
        image_handle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**)&loaded_image,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        return status;
    }

    status = gBS->OpenProtocol(
        loaded_image->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID**)&fs,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        return status;
    }
    
    fs->OpenVolume(fs, root);

    return EFI_SUCCESS;
}

EFI_STATUS OpenGOP(EFI_HANDLE image_handle, EFI_GRAPHICS_OUTPUT_PROTOCOL** gop){
    EFI_STATUS status;
    UINTN num_gop_handles = 0;
    EFI_HANDLE* gop_handles = NULL;

    status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        &num_gop_handles,
        &gop_handles);
    
    if(EFI_ERROR(status)){
        return status;
    }

    status = gBS->OpenProtocol(
        gop_handles[0],
        &gEfiGraphicsOutputProtocolGuid,
        (VOID**)gop,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    
    if(EFI_ERROR(status)){
        return status;
    }

    FreePool(gop_handles);

    return EFI_SUCCESS;
}

void Halt(){
    while(1) __asm__("hlt");
}

const CHAR16* GetPixelFormatUnicode(EFI_GRAPHICS_PIXEL_FORMAT fmt){
    switch(fmt){
        case PixelRedGreenBlueReserved8BitPerColor:
            return L"PixelRedGreenBlueReserved8BitPerColor";
        case PixelBlueGreenRedReserved8BitPerColor:
            return L"PixelBlueGreenRedReserved8BitPerColor";
        case PixelBitMask:
            return L"PixelBitMask";
        case PixelBltOnly:
            return L"PixelBltOnly";
        case PixelFormatMax:
            return L"PixelFormatMax";
        default:
           return L"InvalidPixelFormat";
    }
}

void CalcLoadAddressRange(Elf64_Ehdr* ehdr, UINT64* first, UINT64* last){
    Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64) ehdr+ ehdr->e_phoff);
    *first = MAX_UINT64;
    *last = 0;

    // エルフヘッダからプログラムヘッダを順番に見ていって、
    // アドレス範囲の下限 = 一番若いプログラムヘッダの仮想アドレス
    // アドレス範囲の上限 = 一番大きいプログラムヘッダの仮想アドレス
    //                      ->各仮想アドレス + メモリサイズ
    // を求める
    for(Elf64_Half i = 0; i < ehdr->e_phnum; ++i){
        if(phdr[i].p_type != PT_LOAD) continue;

        *first = MIN(*first, phdr[i].p_vaddr);
        *last = MAX(*last, phdr[i].p_vaddr + phdr[i].p_memsz);
    }
}

void CopyLoadSegments(Elf64_Ehdr* ehdr){
    Elf64_Phdr* phdr = (Elf64_Phdr*)((UINT64) ehdr + ehdr->e_phoff);
    for(Elf64_Half i = 0; i < ehdr->e_phnum; ++i){
        if(phdr[i].p_type != PT_LOAD) continue;

        // 各セグメントのメモリ領域を展開する。
        // .bssセクション(初期値のないグローバル領域)とかは、
        // ファイル上のサイズよりもメモリ上に展開されるべきサイズが大きい場合がある。
        // その場合はゼロ埋めしておく。（->未初期化のグローバル変数がゼロで初期化されるのはこういうことなんかな？）
        UINT64 segm_in_file = (UINT64) ehdr + phdr[i].p_offset;
        CopyMem((VOID*)phdr[i].p_vaddr, (VOID*)segm_in_file, phdr[i].p_filesz);

        UINTN remain_bytes = phdr[i].p_memsz - phdr[i].p_filesz;
        SetMem((VOID*)(phdr[i].p_vaddr + phdr[i].p_filesz), remain_bytes, 0);
    }
}

EFI_STATUS ReadFile(EFI_FILE_PROTOCOL* file, VOID** buffer){
    EFI_STATUS status;

    UINTN file_info_size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 12;
    UINT8 file_info_buffer[file_info_size];

    status = file->GetInfo(
        file, &gEfiFileInfoGuid,
        &file_info_size, file_info_buffer);
    if(EFI_ERROR(status)){
        return status;
    }

    EFI_FILE_INFO* file_info = (EFI_FILE_INFO*)file_info_buffer;
    UINTN file_size = file_info->FileSize;
    
    status = gBS->AllocatePool(EfiLoaderData, file_size, buffer);
    if(EFI_ERROR(status)){
        return status;
    }

    return file->Read(file, &file_size, *buffer);
}

EFI_STATUS OpenBlockIoProtocolForLoadedImage(
    EFI_HANDLE image_handle, EFI_BLOCK_IO_PROTOCOL** block_io){
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL* loaded_image;

    status = gBS->OpenProtocol(
        image_handle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**)&loaded_image,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if(EFI_ERROR(status)){
        return status;
    }

    status = gBS->OpenProtocol(
        loaded_image->DeviceHandle,
        &gEfiBlockIoProtocolGuid,
        (VOID**)block_io,
        image_handle,
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    
    return status;
}

EFI_STATUS ReadBlocks(
    EFI_BLOCK_IO_PROTOCOL* block_io, UINT32 media_id,
    UINTN read_bytes, VOID** buffer){
    EFI_STATUS status;

    status = gBS->AllocatePool(EfiLoaderData, read_bytes, buffer);
    if(EFI_ERROR(status)){
        return status;
    }

    status = block_io->ReadBlocks(
        block_io,
        media_id,
        0,
        read_bytes,
        *buffer);
    return status;
}

EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table){

    EFI_STATUS status;
    
    Print(L"Hello World!\n");

    // メモリマップの読み込み
    CHAR8 memmap_buf[4096 * 4];
    struct MemoryMap memmap = {
        sizeof(memmap_buf),
        memmap_buf,
        0, 0, 0, 0};
    status = GetMemoryMap(&memmap);
    if(EFI_ERROR(status)){
        Print(L"failed to get memory map: %r\n", status);
        Halt();
    }
    
    // ルートディレクトリのオープン
    EFI_FILE_PROTOCOL* root_dir;
    status = OpenRootDir(image_handle, &root_dir);
    if(EFI_ERROR(status)){
        Print(L"failed to open root directory: %r\n", status);
        Halt();
    }

    // メモリマップの読み込み
    EFI_FILE_PROTOCOL* memmap_file;
    status = root_dir->Open(
        root_dir, &memmap_file, L"\\memmap",
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if(EFI_ERROR(status)){
        Print(L"failed to open file '\\memmap': %r\n", status);
        Halt();
    } else {
        status = SaveMemoryMap(&memmap, memmap_file);
        if(EFI_ERROR(status)){
            Print(L"failed to save memory map: %r\n", status);
            Halt();
        }
        status = memmap_file->Close(memmap_file);
        if(EFI_ERROR(status)){
            Print(L"failed to close memory map: %r\n", status);
            Halt();
        }
    }

    // ブートローダでピクセルを塗りつぶす
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    OpenGOP(image_handle, &gop);
    if (EFI_ERROR(status)){
        Print(L"failed to open GOP* %r\n", status);
    }

    // Kernelファイルのオープン
    EFI_FILE_PROTOCOL* kernel_file;
    status = root_dir->Open(root_dir, &kernel_file, L"\\Kernel.elf", EFI_FILE_MODE_READ, 0);
    if(EFI_ERROR(status)){
        Print(L"failed to open file '\\kernel.elf': %r\n", status);
        Halt();
    }

    VOID* kernel_buffer;
    status = ReadFile(kernel_file, &kernel_buffer);
    if(EFI_ERROR(status)){
        Print(L"error: %r", status);
        Halt();
    }
    // Kernel用ページの割当
    Elf64_Ehdr* kernel_ehdr = (Elf64_Ehdr*)kernel_buffer;
    UINT64 kernel_first_addr, kernel_last_addr;
    CalcLoadAddressRange(kernel_ehdr, &kernel_first_addr, &kernel_last_addr);

    UINTN num_pages = (kernel_last_addr - kernel_first_addr + 0xfff) / 0x1000;
    status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, num_pages, &kernel_first_addr);
    if(EFI_ERROR(status)){
        Print(L"failed to allocate pages: %r\n", status);
        Halt();
    }

    CopyLoadSegments(kernel_ehdr);
    Print(L"Kerenel : 0x%0lx (%lu bytes)\n", kernel_first_addr, kernel_last_addr);

    status = gBS->FreePool(kernel_buffer);
    if(EFI_ERROR(status)){
        Print(L"failed to free pool: %r\n", status);
        Halt();
    }

    VOID* volume_image;
    EFI_FILE_PROTOCOL* volume_file;
    status = root_dir->Open(
        root_dir, &volume_file, L"\\fat_disk",
        EFI_FILE_MODE_READ, 0);
    if(status == EFI_SUCCESS){
        status = ReadFile(volume_file, &volume_image);
        if(EFI_ERROR(status)){
            Print(L"failed to read volume file: %r", status);
            Halt();
        }
    } else {
        EFI_BLOCK_IO_PROTOCOL* block_io;
        status = OpenBlockIoProtocolForLoadedImage(image_handle, &block_io);
        if(EFI_ERROR(status)){
            Print(L"failed to open Block I/O Protcol: %r", status);
            Halt();           
        }
        EFI_BLOCK_IO_MEDIA* media = block_io->Media;
        UINTN volume_bytes = (UINTN)media->BlockSize * (media->LastBlock + 1);
        if(volume_bytes > 32 * 1024 * 1024){
            volume_bytes = 32 * 1024 *1024; 
        }

        Print(L"Reading %lu bytes (Present %d, Blocksize %u, LastBlock %u)\n",
                volume_bytes, media->MediaPresent, media->BlockSize, media->LastBlock);
        
        status = ReadBlocks(block_io, media->MediaId, volume_bytes, &volume_image);
        if(EFI_ERROR(status)){
            Print(L"failed to read blocks: %r\n", status);
            Halt();
        }
    }



    // ブートローダを停止する。
    status = gBS->ExitBootServices(image_handle, memmap.map_key);
    if(EFI_ERROR(status)){
        status = GetMemoryMap(&memmap);
        if(EFI_ERROR(status)){
            Print(L"faile to get memory map: %r\n", status);
            Halt();
        }
        status = gBS->ExitBootServices(image_handle, memmap.map_key);
        if(EFI_ERROR(status)){
            Print(L"could not exit boot service: %r\n", status);
            Halt();
        }
    }

    // Kernelを起動する。
    UINT64 entry_addr = *(UINT64*)(kernel_first_addr + 24);
    
    struct FrameBufferConfig config = {
        (UINT8*)gop->Mode->FrameBufferBase,
        gop->Mode->Info->PixelsPerScanLine,
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        0
    };

    switch(gop->Mode->Info->PixelFormat){
        case PixelRedGreenBlueReserved8BitPerColor:
            config.pixel_format = kPixelRGBResv8BitPerColor;
            break;
        case PixelBlueGreenRedReserved8BitPerColor:
            config.pixel_format = kPixelBGRResv8BitPerColor;
            break;
        default:
            Print(L"Unimplemented pixel format: %d\n", gop->Mode->Info->PixelFormat);
            Halt();
            break;
    }

    VOID* acpi_table = NULL;
    for(UINTN i = 0; i < system_table->NumberOfTableEntries; ++i){
        if(CompareGuid(&gEfiAcpiTableGuid,
                        &system_table->ConfigurationTable[i].VendorGuid)){
            acpi_table = system_table->ConfigurationTable[i].VendorTable;
            break;
        }
    }

    typedef void EntryPointType(const struct FrameBufferConfig*,
                                const struct MemoryMap*,
                                const VOID*,
                                VOID*);
    EntryPointType* entry_point = (EntryPointType*)entry_addr;
    entry_point(&config, &memmap, acpi_table, volume_image);

    Print(L"Error : Kernel cant load.\n");

    while(1);
    return EFI_SUCCESS;
}