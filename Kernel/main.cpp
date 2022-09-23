#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

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

    for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x){
        for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y){
            pixel_writer->Write(x, y, {255, 255, 255});
        }
    }

    const int kFrameWidth = frame_buffer_config.horizontal_resolution;
    const int kFrameHight = frame_buffer_config.vertical_resolution;

    // 背景の描画
    FillRectangle(*pixel_writer,
                    {0, 0},
                    {kFrameWidth, kFrameHight - 50},
                    kDesktopBGColor);

    // タスクバーの描画
    FillRectangle(*pixel_writer,
                    {0, kFrameHight - 50},
                    {kFrameWidth, 50},
                    {1, 8, 17});

    // スタートボタンのあたり？
    FillRectangle(*pixel_writer,
                    {0, kFrameHight - 50},
                    {kFrameWidth / 5, 50},
                    {80, 80, 80});

    // スタートボタンの枠？
    FillRectangle(*pixel_writer,
                    {10, kFrameHight - 40},
                    {30, 30},
                    {160, 160, 160});

    // マウスカーソルの描画
    for(int dy = 0; dy < kMouseCursorHeight; ++dy){
        for(int dx = 0; dx < kMouseCursorWidth; ++dx){
            if(mouse_cursor_shape[dy][dx] == '@'){
                pixel_writer->Write(200 + dx, 100 + dy, {0, 0, 0});
            } else if(mouse_cursor_shape[dy][dx] == '.'){
                pixel_writer->Write(200 + dx, 100 + dy, {255, 255, 255});
            }
        }
    }

    console = new(console_buf) Console{*pixel_writer, kDesktopFGColor, kDesktopBGColor};
    printk("Welcome to MikanOS_X\n");

    while(1) __asm__("hlt");
}