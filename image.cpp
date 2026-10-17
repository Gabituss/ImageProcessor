#include "image.h"

void Color::Normalize() {
    r = std::clamp(r, 0.0f, 1.0f);
    g = std::clamp(g, 0.0f, 1.0f);
    b = std::clamp(b, 0.0f, 1.0f);
}

Color operator*(float x, const Color &color) {
    return Color(color.r * x, color.g * x, color.b * x);
}

Color operator+(const Color &a, const Color &b) {
    return Color(a.r + b.r, a.g + b.g, a.b + b.b);
}