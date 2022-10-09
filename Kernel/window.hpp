#pragma once

#include "graphics.hpp"
#include "frame_buffer.hpp"
#include "logger.hpp"

#include <vector>
#include <optional>

class Window {
    public:
        class WindowWriter : public PixelWriter {
            public:
                WindowWriter(Window& window) : window_{window} {}
                virtual void Write(Vector2D<int> pos, const PixelColor& c) override{
                    window_.Write(pos, c);
                }
                virtual int Width() const override { return window_.Width(); }
                virtual int Height() const override { return window_.Height(); }
            private:
                Window& window_;
        };

        Window(int width, int height, PixelFormat shadow_format);
        ~Window() = default;
        Window(const Window& rhs) = delete;
        Window& operator=(const Window& rhs) = delete;

        void DrawTo(FrameBuffer& dst, Vector2D<int> position);
        void SetTransparentColor(std::optional<PixelColor> c);
        WindowWriter* Writer();

        const PixelColor& At(Vector2D<int> pos) const;
        void Write(Vector2D<int> pos, PixelColor c);

        int Width() const ;
        int Height() const ;
    
    private:
        int width_, height_;
        std::vector<std::vector<PixelColor>> data_{};
        WindowWriter writer_{*this};
        std::optional<PixelColor> transparent_color_{std::nullopt};

        FrameBuffer shadow_buffer_{};
};