#pragma once

#include <cstdint>
#include "graphics.hpp"

extern const uint8_t kFontA[16];
void WriteAscii(PixelWriter& writer, Vector2D<int> pos, char c, const PixelColor& color);
void WriteString(PixelWriter& writer, Vector2D<int> pos, const char* s, const PixelColor& color);
