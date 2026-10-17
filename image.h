#pragma once

#include "cstddef"
#include "vector"

struct Color {
    float r = 0;
    float g = 0;
    float b = 0;

    Color() = default;

    Color(float r, float g, float b) : r(r), g(g), b(b) {
    }

    void Normalize();
};

Color operator*(float x, const Color &color);
Color operator+(const Color &a, const Color &b);

class Image {
public:
    Image(int32_t width, int32_t height) : width_(width), height_(height), pixels_(width * height) {
    }

    int32_t Width() const {
        return width_;
    }

    int32_t Height() const {
        return height_;
    }

    Color &At(size_t x, size_t y) {
        return pixels_[y * width_ + x];
    }

    const Color &At(size_t x, size_t y) const {
        return pixels_[y * width_ + x];
    }

private:
    int32_t width_;
    int32_t height_;
    std::vector<Color> pixels_;
};
