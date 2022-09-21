#pragma once

#include <cstdint>
#include "graphics.hpp"

extern const uint8_t kFontA[16];
void WriteAscii(PixelWriter& writer, int x, int y, char c, const PixelColor& color);
void WriteString(PixelWriter& writer, int x, int y ,const char* s, const PixelColor& color);
