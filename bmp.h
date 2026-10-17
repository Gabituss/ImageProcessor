#pragma once

#include <cstddef>
#include <string_view>
#include "image.h"

namespace bmp {
Image Read(std::string_view path);

void Write(const Image& image, std::string_view path);

}  // namespace bmp