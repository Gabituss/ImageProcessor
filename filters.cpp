#include "filters.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "exceptions.h"

namespace {

float StringToFloat(std::string_view s) {
    size_t pos = 0;
    float value = 0;

    try {
        value = std::stof(std::string(s), &pos);
    } catch (const std::exception&) {
        throw ArgumentConversionError("cannot cast string to float");
    }

    if (pos != s.size()) {
        throw ArgumentConversionError("cannot cast string to float");
    }

    if (!std::isfinite(value)) {
        throw ArgumentConversionError("excepted finite number");
    }

    return value;
}

int32_t StringToInt(std::string_view s) {
    size_t pos = 0;
    int32_t value = 0;
    try {
        value = std::stoi(std::string(s), &pos);
    } catch (const std::exception&) {
        throw ArgumentConversionError("cannot cast string to int");
    }

    if (pos != s.size()) {
        throw ArgumentConversionError("cannot cast string to int");
    }

    return value;
}

} 

GrayscaleFilter::GrayscaleFilter() {
}
GrayscaleFilter::GrayscaleFilter([[maybe_unused]] const FilterSpec& spec) {
}

void GrayscaleFilter::Transform(Image& image) const {
    for (int32_t y = 0; y < image.Height(); y++) {
        for (int32_t x = 0; x < image.Width(); x++) {
            const Color value = image.At(x, y);

            float pixel_color = value.r * RCoefficient + value.g * GCoefficient + value.b * BCoefficient;

            image.At(x, y) = Color(pixel_color, pixel_color, pixel_color);
        }
    }
}

void GrayscaleFilter::CheckArgs(const FilterSpec& spec) {
    if (!spec.params.empty()) {
        throw TooManyArgumentsError("too many arguments in grayscale filter");
    }
};

NegativeFilter::NegativeFilter([[maybe_unused]] const FilterSpec& spec) {
}

void NegativeFilter::Transform(Image& image) const {
    for (int32_t y = 0; y < image.Height(); y++) {
        for (int32_t x = 0; x < image.Width(); x++) {
            const Color value = image.At(x, y);

            image.At(x, y) = Color(1 - value.r, 1 - value.g, 1 - value.b);
        }
    }
}

void NegativeFilter::CheckArgs(const FilterSpec& spec) {
    if (!spec.params.empty()) {
        throw TooManyArgumentsError("too many arguments in negative filter");
    }
};

MatrixFilter::MatrixFilter() : MatrixFilter({{1}}, true) {
}

MatrixFilter::MatrixFilter(std::vector<std::vector<float>> matrix, bool normalize) : matrix_(std::move(matrix)) {
    height_ = static_cast<int32_t>(matrix_.size());
    width_ = static_cast<int32_t>(matrix_[0].size());
    normalize_ = normalize;

    if (height_ % 2 == 0) {
        throw FilterInitError("wrong height in matrix filter");
    }

    if (width_ % 2 == 0) {
        throw FilterInitError("wrong width in matrix filter");
    }
}

Color MatrixFilter::GetInPoint(const Image& image, int32_t x, int32_t y) const {
    Color result;

    for (int32_t dy = -(height_ / 2); dy <= height_ / 2; dy++) {
        for (int32_t dx = -(width_ / 2); dx <= width_ / 2; dx++) {
            int32_t nx = std::clamp<int32_t>(x + dx, 0, image.Width() - 1);
            int32_t ny = std::clamp<int32_t>(y + dy, 0, image.Height() - 1);

            result = result + matrix_[dy + height_ / 2][dx + width_ / 2] * image.At(nx, ny);
        }
    }

    return result;
}

void MatrixFilter::Apply(Image& image) const {
    Image nw(image.Width(), image.Height());

    for (int32_t y = 0; y < image.Height(); y++) {
        for (int32_t x = 0; x < image.Width(); x++) {
            nw.At(x, y) = GetInPoint(image, x, y);

            if (normalize_) {
                nw.At(x, y).Normalize();
            }       
        }
    }

    image = std::move(nw);
}

SharpeningFilter::SharpeningFilter() : filter_(matrix_) {
}

SharpeningFilter::SharpeningFilter([[maybe_unused]] const FilterSpec& spec) : SharpeningFilter() {
}

void SharpeningFilter::Apply(Image& image) const {
    filter_.Apply(image);
}

void SharpeningFilter::CheckArgs(const FilterSpec& spec) {
    if (!spec.params.empty()) {
        throw TooManyArgumentsError("too many arguments in sharpening filter");
    }
};

ThresholdFilter::ThresholdFilter(float threshold) : threshold_(threshold) {
};

void ThresholdFilter::Transform(Image &image) const {
    for (int32_t y = 0; y < image.Height(); y++) {
        for (int32_t x = 0; x < image.Width(); x++) {
            float value = image.At(x, y).r;

            if (value > threshold_) {
                image.At(x, y) = Color(1, 1, 1);
            } else {
                image.At(x, y) = Color(0, 0, 0);
            }
        }
    }
}

EdgeDetectionFilter::EdgeDetectionFilter(const FilterSpec& spec) {
    float threshold = ParseThreshold(spec);
    filters_.emplace_back(std::make_unique<GrayscaleFilter>());
    filters_.emplace_back(std::make_unique<MatrixFilter>(
        std::vector<std::vector<float>>{
            {0.0f, -1.0f, 0.0f},
            {-1.0f, 4.0f, -1.0f},
            {0.0, -1.0f, 0.0f}})
        );
    filters_.emplace_back(std::make_unique<ThresholdFilter>(threshold));
}

float EdgeDetectionFilter::ParseThreshold(const FilterSpec& spec) {
    return StringToFloat(spec.params[0]);
}

void EdgeDetectionFilter::CheckArgs(const FilterSpec& spec) {
    if (spec.params.empty()) {
        throw NotEnoughArgumentsError("not enought arguments in edge detection filter");
    }
    if (spec.params.size() > 1) {
        throw TooManyArgumentsError("too many arguments in edge detection filter");
    }
};

GaussianBlurFilter::GaussianBlurFilter(const FilterSpec& spec) {
    float sigma = ParseSigma(spec);
    filters_.emplace_back(std::make_unique<MatrixFilter>(Vertical(sigma), false));
    filters_.emplace_back(std::make_unique<MatrixFilter>(Horizontal(sigma), true));
}

float GaussianBlurFilter::ParseSigma(const FilterSpec& spec) {
    return StringToFloat(spec.params[0]);
}

void GaussianBlurFilter::CheckArgs(const FilterSpec& spec) {
    if (spec.params.empty()) {
        throw NotEnoughArgumentsError("not enought arguments in gaussian filter");
    }
    if (spec.params.size() > 1) {
        throw TooManyArgumentsError("too many arguments in gaussian filter");
    }

    float sigma = ParseSigma(spec);
    if (sigma <= 0 || sigma > MaximalSigma) {
        throw ValueOutOfRange("value of sigma is out of range");
    }
};

int32_t GaussianBlurFilter::Radius(float sigma) {
    return static_cast<int32_t>(std::ceil(3 * sigma));
}

std::vector<float> GaussianBlurFilter::Weights(float sigma) {
    int32_t r = Radius(sigma);

    std::vector<float> weights;
    float sum = 0;

    for (int32_t i = -r; i <= r; ++i) {
        const double ratio = static_cast<double>(i) / sigma;
        const float w = static_cast<float>(std::exp(-0.5 * ratio * ratio));

        weights.push_back(w);
        sum += w;
    }

    for (float& w : weights) {
        w /= sum;
    }

    return weights;
}

std::vector<std::vector<float>> GaussianBlurFilter::Horizontal(float sigma) {
    return {Weights(sigma)};
}

std::vector<std::vector<float>> GaussianBlurFilter::Vertical(float sigma) {
    std::vector<std::vector<float>> result;

    for (float w : Weights(sigma)) {
        result.push_back({w});
    }

    return result;
}

CropFilter::CropFilter(const FilterSpec& spec) : CropFilter(ParseWidth(spec), ParseHeight(spec)) {
}

CropFilter::CropFilter(int32_t width, int32_t height) : width_(width), height_(height) {
}

void CropFilter::Apply(Image& image) const {
    Image result(std::min(width_, image.Width()), std::min(height_, image.Height()));

    for (int32_t y = image.Height() - result.Height(); y < image.Height(); y++) {
        for (int32_t x = 0; x < result.Width(); x++) {
            result.At(x, result.Height() - image.Height() + y) = image.At(x, y);
        }
    }

    image = std::move(result);
}

int32_t CropFilter::ParseWidth(const FilterSpec& spec) {
    return StringToInt(spec.params[0]);
}

int32_t CropFilter::ParseHeight(const FilterSpec& spec) {
    return StringToInt(spec.params[1]);
}

void CropFilter::CheckArgs(const FilterSpec& spec) {
    if (spec.params.size() < 2) {
        throw NotEnoughArgumentsError("not enought arguments in crop filter");
    }
    if (spec.params.size() > 2) {
        throw TooManyArgumentsError("too many arguments in crop filter");
    }

    int height = ParseHeight(spec);
    int width = ParseWidth(spec);

    if (height <= 0 || width <= 0) {
        throw ValueOutOfRange("Value of height or width is out of range in crop filter");
    }
};

VoronoiFilter::VoronoiFilter(const FilterSpec& spec) : VoronoiFilter(ParseNumberOfPoints(spec)) {
}

VoronoiFilter::VoronoiFilter(int number_of_points) : p_cnt_(number_of_points) {
}

constexpr uint32_t RandomSeed = 6767676767;

void VoronoiFilter::Apply(Image& image) const {
    int32_t w = image.Width();
    int32_t h = image.Height();

    std::mt19937 rng(RandomSeed);

    float area = static_cast<float>(w) * static_cast<float>(h);
    int32_t s = std::max(1, static_cast<int>(std::sqrt(area / static_cast<float>(p_cnt_))));

    int32_t cols = (w - 1) / s + 1;
    int32_t rows = (h - 1) / s + 1;

    std::vector<std::pair<int32_t, int32_t>> points;
    std::vector<size_t> cnt;
    std::vector<Color> sum_colors;


    std::vector<std::vector<size_t>> nearest(image.Width(), std::vector<size_t>(image.Height()));

    for (int cy = 0; cy < rows; ++cy) {
        for (int cx = 0; cx < cols; ++cx) {
            std::uniform_int_distribution<int32_t> gen_x(cx * s, std::min(cx * s + s - 1, w - 1));
            std::uniform_int_distribution<int32_t> gen_y(cy * s, std::min(cy * s + s - 1, h - 1));

            int32_t x = gen_x(rng);
            int32_t y = gen_y(rng);

            points.push_back({x, y});
            cnt.push_back(0);
            sum_colors.push_back(Color(0,0,0));
        }
    }

    for (int32_t y = 0; y < h; ++y) {
        for (int32_t x = 0; x < w; ++x) {
            int32_t cx = x / s;
            int32_t cy = y / s;

            int32_t best_dist = std::numeric_limits<int32_t>::max();
            size_t best_id = 0;

            for (int32_t dy = -3; dy <= 3; ++dy) {
                for (int32_t dx = -3; dx <= 3; ++dx) {
                    int32_t nx = cx + dx;
                    int32_t ny = cy + dy;

                    if (nx < 0 || nx >= cols ||
                        ny < 0 || ny >= rows) {
                        continue;
                    }

                    size_t id = static_cast<size_t>(ny) * cols + nx;
                    auto [px,py] = points[id];

                    int32_t dist = (x - px) * (x - px) + (y - py) * (y - py);

                    if (dist < best_dist) {
                        best_dist = dist;
                        best_id = id;
                    }
                }
            }
            
            nearest[x][y] = best_id;
            cnt[best_id]++;
            sum_colors[best_id] = sum_colors[best_id] + image.At(x, y);
        }
    }
    
    for (size_t i = 0; i < cnt.size(); i++) {
        sum_colors[i] = (1.0f / static_cast<float>(cnt[i])) * sum_colors[i];
    }
    
    for (int32_t y = 0; y < h; ++y) {
        for (int32_t x = 0; x < w; ++x) {
            image.At(x, y) = sum_colors[nearest[x][y]];
        }
    }
}

int32_t VoronoiFilter::ParseNumberOfPoints(const FilterSpec& spec) {
    return StringToInt(spec.params[0]);
}

void VoronoiFilter::CheckArgs(const FilterSpec& spec) {
    if (spec.params.empty()) {
        throw NotEnoughArgumentsError("not enought arguments in voronoi filter");
    }
    if (spec.params.size() > 1) {
        throw TooManyArgumentsError("too many arguments in voronoi filter");
    }

    int32_t number_of_points = ParseNumberOfPoints(spec);
    if (number_of_points <= 0) {
        throw ValueOutOfRange("number of points is out of range in voronoi filter");
    }
}