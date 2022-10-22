#include "mouse.hpp"

#include "graphics.hpp"
#include "layer.hpp"
#include "usb/classdriver/mouse.hpp"

namespace {
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
}

void DrawMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position) {
    for(int dy = 0; dy < kMouseCursorHeight; ++dy){
        for(int dx = 0; dx < kMouseCursorWidth; ++dx){
            if(mouse_cursor_shape[dy][dx] == '@'){
                pixel_writer->Write(position + Vector2D<int>{dx, dy}, {0, 0, 0});
            } else if(mouse_cursor_shape[dy][dx] == '.'){
                pixel_writer->Write(position + Vector2D<int>{dx, dy}, {255, 255, 255});
            } else {
                pixel_writer->Write(position + Vector2D<int>{dx, dy}, kMouseTransparentColor);
            }
        }
    }
}

Mouse::Mouse(unsigned int layer_id) : layer_id_ {layer_id} {

}

void Mouse::SetPosition(Vector2D<int> position){
    position_ = position;
    layer_manager->Move(layer_id_, position_);
}

void Mouse::OnInterrupt(uint8_t buttons, int8_t displacement_x, int8_t displacement_y){
        static unsigned int mouse_drag_layer_id = 0;
    static uint8_t previous_buttons = 0;

    const auto oldpos = position_;
    auto newpos = position_ + Vector2D<int> {displacement_x, displacement_y};
    newpos = ElementMin(newpos, ScreenSize() + Vector2D<int>{-1, -1});
    position_ = ElementMax(newpos, {0, 0});

    const auto possdiff = position_ - oldpos;

    layer_manager->Move(layer_id_, position_);

    const bool previous_left_pressed = (previous_buttons & 0x01);
    const bool left_pressed = (buttons & 0x01);
    if(!previous_left_pressed && left_pressed){
        // クリック

        // クリック位置からウィンドウ（＝レイヤ）を探して、ドラッグ中のレイヤIDとして覚える。
        auto layer = layer_manager->FindLayerByPosition(position_, layer_id_);
        if(layer && layer->IsDraggable()){
            mouse_drag_layer_id = layer->ID();
        }
    } else if(previous_left_pressed && left_pressed){
        // ドラッグ中
        if(mouse_drag_layer_id > 0){
            layer_manager->MoveRelative(mouse_drag_layer_id, possdiff);
        }
    } else if(previous_buttons && !left_pressed){
        // リリース
        mouse_drag_layer_id = 0;
    }

    previous_buttons = buttons;
}

std::shared_ptr<Mouse> MakeMouse() {
    auto mouse_window = std::make_shared<Window>(
        kMouseCursorWidth, kMouseCursorHeight, screen_config.pixel_format);
    mouse_window->SetTransparentColor(kMouseTransparentColor);
    DrawMouseCursor(mouse_window->Writer(), {0, 0});

    auto mouse_layer_id = layer_manager->NewLayer()
                        .SetWindow(mouse_window)
                        .ID();

    auto mouse = std::make_shared<Mouse>(mouse_layer_id);
    mouse->SetPosition({200, 200});
    usb::HIDMouseDriver::default_observer =
        [mouse](uint8_t buttons, int8_t displacement_x, int8_t displacement_y) {
        mouse->OnInterrupt(buttons, displacement_x, displacement_y);
    };

    return mouse;
}