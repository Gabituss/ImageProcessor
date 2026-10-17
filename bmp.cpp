#include <cstdint>
#include <fstream>
#include "bmp.h"
#include "exceptions.h"

namespace bmp {
namespace {
constexpr uint16_t KSignature = 0x4D42;  // BM
constexpr uint16_t BitsPerPixel = 24;
constexpr uint32_t FileHeaderSize = 14;
constexpr uint32_t InfoHeaderSize = 40;
constexpr uint8_t MaxValue = 255;
constexpr char Zeros[4] = {};

template <typename T>
T ReadTo(std::istream& in) {
    T value;
    if (!in.read(reinterpret_cast<char*>(&value), sizeof(T))) {
        throw BmpReadingError("Not enought bytes to read");
    }
    return value;
}

template <typename T>
void WriteTo(T value, std::ofstream& out) {
    if (!out.write(reinterpret_cast<char*>(&value), sizeof(T))) {
        throw BmpWritingError("Cannot write to a file");
    }
}
}  // namespace

Image Read(std::string_view path) {
    std::ifstream in(std::string(path), std::ios::binary);
    if (!in) {
        throw BmpOpenError("cannot open file");
    }

    uint16_t signature = ReadTo<uint16_t>(in);
    if (signature != KSignature) {
        throw BmpProvidedNotABmpError("wrong signature");
    }

    [[maybe_unused]] uint32_t file_size = ReadTo<uint32_t>(in);
    [[maybe_unused]] uint32_t reserved = ReadTo<uint32_t>(in);
    uint32_t offset = ReadTo<uint32_t>(in);

    [[maybe_unused]] uint32_t header_size = ReadTo<uint32_t>(in);

    int32_t width = ReadTo<int32_t>(in);
    int32_t height = ReadTo<int32_t>(in);

    if (width <= 0 || height <= 0) {
        throw BmpReadingError("unsupported BMP image size");
    }

    [[maybe_unused]] uint16_t planes = ReadTo<uint16_t>(in);
    uint16_t bit_count = ReadTo<uint16_t>(in);

    if (bit_count != BitsPerPixel) {
        throw BmpReadingError("wrong pixel size");
    }

    uint32_t compression = ReadTo<uint32_t>(in);
    if (compression != 0) {
        throw BmpReadingError("wrong compression");
    }

    [[maybe_unused]] uint32_t image_size = ReadTo<uint32_t>(in);
    [[maybe_unused]] uint32_t x_pels_per_meter = ReadTo<uint32_t>(in);
    [[maybe_unused]] uint32_t y_pels_per_meter = ReadTo<uint32_t>(in);
    [[maybe_unused]] uint32_t color_used = ReadTo<uint32_t>(in);
    [[maybe_unused]] uint32_t color_important = ReadTo<uint32_t>(in);

    in.seekg(offset, std::ios::beg);
    if (!in) {
        throw BmpReadingError("cannot seek to pixel data");
    }

    uint32_t padding = (4 - (width * 3) % 4) % 4;

    Image image(width, height);
    for (int32_t y = 0; y < height; y++) {
        for (int32_t x = 0; x < width; x++) {
            uint8_t b = ReadTo<uint8_t>(in);
            uint8_t g = ReadTo<uint8_t>(in);
            uint8_t r = ReadTo<uint8_t>(in);

            image.At(x, y) = Color(static_cast<float>(r) / MaxValue, static_cast<float>(g) / MaxValue,
                                   static_cast<float>(b) / MaxValue);
        }

        in.ignore(padding);
    }

    in.close();

    return image;
}

void Write(const Image& image, std::string_view path) {
    std::ofstream out(std::string(path), std::ios::binary);

    if (!out) {
        throw BmpOpenError("cannot open file");
    }

    uint32_t padding = (4 - (image.Width() * 3) % 4) % 4;
    uint32_t size_image = (image.Width() * 3 + padding) * image.Height();

    WriteTo(KSignature, out);                                              // signature
    WriteTo<uint32_t>(FileHeaderSize + InfoHeaderSize + size_image, out);  // size
    WriteTo<uint32_t>(0, out);                                             // reserved
    WriteTo<uint32_t>(FileHeaderSize + InfoHeaderSize, out);               // offset

    WriteTo<uint32_t>(InfoHeaderSize, out);
    WriteTo<uint32_t>(image.Width(), out);
    WriteTo<uint32_t>(image.Height(), out);
    WriteTo<uint16_t>(1, out);  // planes
    WriteTo<uint16_t>(BitsPerPixel, out);
    WriteTo<uint32_t>(0, out);  // compression
    WriteTo<uint32_t>(size_image, out);
    WriteTo<uint32_t>(0, out);  // xPelsPerMeter
    WriteTo<uint32_t>(0, out);  // yPelsPerMeter
    WriteTo<uint32_t>(0, out);  // colorUsed
    WriteTo<uint32_t>(0, out);  // colorImportant

    for (int32_t y = 0; y < image.Height(); y++) {
        for (int32_t x = 0; x < image.Width(); x++) {
            const Color color = image.At(x, y);
            WriteTo<uint8_t>(static_cast<uint8_t>(color.b * MaxValue), out);
            WriteTo<uint8_t>(static_cast<uint8_t>(color.g * MaxValue), out);
            WriteTo<uint8_t>(static_cast<uint8_t>(color.r * MaxValue), out);
        }

        out.write(Zeros, padding);
        if (!out) {
            throw BmpWritingError("cannot write row padding");
        }
    }

    out.close();
    if (!out) {
        throw BmpWritingError("cannot finish writing BMP");
    }
}
}  // namespace bmp