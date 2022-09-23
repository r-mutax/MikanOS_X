#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
   //123456789ABCDEF 
    "@              ",
    "@@             ",
    "@.@            ",
    "@..@           ",
    "@...@          ",
    "@....@         ",
    "@.....@        ",
    "@......@       ",
    "@.......@      ",
    "@........@     ",
    "@.........@    ",
    "@..........@   ",
    "@...........@  ",
    "@............@ ",
    "@......@@@@@@@@",
    "@......@       ",
    "@....@@.@      ",
    "@...@ @.@      ",
    "@..@   @.@     ",
    "@.@    @.@     ",
    "@@      @.@    ",
    "@       @.@    ",
    "         @.@   ",
    "         @@@   "
};

void* operator new(size_t size, void* buf) {
  return buf;
}

void operator delete(void* obj) noexcept {
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

extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config){

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

    console = new(console_buf) Console{*pixel_writer, {0, 0, 0}, {255, 255, 255}};

    for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x){
        for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y){
            pixel_writer->Write(x, y, {255, 255, 255});
        }
    }

    for(int dy = 0; dy < kMouseCursorHeight; ++dy){
        for(int dx = 0; dx < kMouseCursorWidth; ++dx){
            if(mouse_cursor_shape[dy][dx] == '@'){
                pixel_writer->Write(200 + dx, 100 + dy, {0, 0, 0});
            } else if(mouse_cursor_shape[dy][dx] == '.'){
                pixel_writer->Write(200 + dx, 100 + dy, {255, 255, 255});
            }
        }
    }

    while(1) __asm__("hlt");
}