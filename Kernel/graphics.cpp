#include "graphics.hpp"

void RGBResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor& c) {
    auto p = PixelAt(x, y);
    p[0] = c.r;
    p[1] = c.g;
    p[2] = c.b;
}

void BGRResv8BitPerColorPixelWriter::Write(int x, int y, const PixelColor& c) {
    auto p = PixelAt(x, y);
    p[0] = c.b;
    p[1] = c.g;
    p[2] = c.r;
}

void FillRectangle(PixelWriter& writer, const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c){
    for (int dy = 0; dy < size.y; ++dy){
        for (int dx = 0; dx < size.x; ++dx){
            writer.Write(dx + pos.x, dy + pos.y, c);
        }
    }
}

void DrawRectangle(PixelWriter& writer, const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c){
    for(int dx = 0; dx < size.x; ++dx){
        writer.Write(dx + pos.x, pos.y, c);
        writer.Write(dx + pos.x, pos.y + size.y - 1, c);
    }

    for(int dy = 0; dy < size.y; ++dy){
        writer.Write(pos.x, dy + pos.y, c);
        writer.Write(pos.x + size.x - 1, dy + pos.y, c);
    }
}