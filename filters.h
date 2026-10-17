#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "args.h"
#include "image.h"

class Filter {
public:
    Filter() = default;
    virtual ~Filter() = default;

    virtual void Apply(Image& image) const = 0;

    static void CheckArgs(const FilterSpec&);
};

class PixelFilter : public Filter {
public:
    virtual void Transform(Image& image) const = 0;

    void Apply(Image& image) const override {
        Transform(image);
    }
};

class GrayscaleFilter : public PixelFilter {
public:
    GrayscaleFilter();
    explicit GrayscaleFilter(const FilterSpec& spec);

    void Transform(Image& image) const override;

    static void CheckArgs(const FilterSpec&);

private:
    static constexpr float RCoefficient = 0.299f;
    static constexpr float GCoefficient = 0.587f;
    static constexpr float BCoefficient = 0.114f;
};

class NegativeFilter : public PixelFilter {
public:
    explicit NegativeFilter(const FilterSpec& spec);

    void Transform(Image& image) const override;

    static void CheckArgs(const FilterSpec&);
};

class MatrixFilter : public Filter {
public:
    MatrixFilter();
    explicit MatrixFilter(std::vector<std::vector<float>> matrix, bool normalize=true);

    Color GetInPoint(const Image& image, int32_t x, int32_t y) const;

    void Apply(Image& image) const override;

private:
    int32_t height_;
    int32_t width_;
    bool normalize_ = true;
    std::vector<std::vector<float>> matrix_;
};

class SharpeningFilter : public Filter {
public:
    SharpeningFilter();
    explicit SharpeningFilter(const FilterSpec& spec);

    void Apply(Image& image) const override;

    static void CheckArgs(const FilterSpec&);

private:
    const std::vector<std::vector<float>> matrix_ = {{0, -1, 0}, {-1, 5, -1}, {0, -1, 0}};

    const MatrixFilter filter_;
};

class EdgeDetectionFilter : public Filter {
public:
    explicit EdgeDetectionFilter(const FilterSpec& spec);
    explicit EdgeDetectionFilter(float threshold);

    void Apply(Image& image) const override;

    static void CheckArgs(const FilterSpec&);

private:
    static float ParseThreshold(const FilterSpec& spec);

    const float treshold_ = 0;
    const std::vector<std::vector<float>> matrix_ = {{0, -1, 0}, {-1, 4, -1}, {0, -1, 0}};

    MatrixFilter filter_matrix_;
    GrayscaleFilter filter_grayscale_;
};

class GaussianBlurFilter : public Filter {
public:
    explicit GaussianBlurFilter(const FilterSpec& spec);
    explicit GaussianBlurFilter(float sigma);

    void Apply(Image& image) const override;

    static void CheckArgs(const FilterSpec&);

private:
    static float ParseSigma(const FilterSpec& spec);
    static constexpr float MaximalSigma = 1000;
    int32_t Radius(float sigma);
    std::vector<float> Weights(float sigma);
    std::vector<std::vector<float>> Horizontal(float sigma);
    std::vector<std::vector<float>> Vertical(float sigma);

    const MatrixFilter filter_vertical_;
    const MatrixFilter filter_horizontal_;
};

class CropFilter : public Filter {
public:
    explicit CropFilter(const FilterSpec& spec);
    CropFilter(int32_t width, int32_t height);

    void Apply(Image& image) const override;

    static void CheckArgs(const FilterSpec&);

private:
    static int32_t ParseWidth(const FilterSpec& spec);
    static int32_t ParseHeight(const FilterSpec& spec);

    const int32_t width_;
    const int32_t height_;
};

class VoronoiFilter : public Filter {
public:
    explicit VoronoiFilter(const FilterSpec& spec);
    explicit VoronoiFilter(int number_of_points);

    void Apply(Image& image) const override;

    static void CheckArgs(const FilterSpec&);

private:
    static int32_t ParseNumberOfPoints(const FilterSpec& spec);

    size_t p_cnt_;
};