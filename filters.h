#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
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

class CompositeFilter : public Filter {
public:
    CompositeFilter() = default;
    virtual ~CompositeFilter() = default;

    void Apply(Image &image) const { 
        for (auto &filter: filters_) {
            filter->Apply(image);
        }
    }

    std::vector<std::unique_ptr<Filter>> filters_;
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

class SharpeningFilter : public MatrixFilter {
public:
    SharpeningFilter();
    explicit SharpeningFilter(const FilterSpec& spec);

    void Apply(Image& image) const override;

    static void CheckArgs(const FilterSpec&);

private:
    const std::vector<std::vector<float>> matrix_ = {{0, -1, 0}, {-1, 5, -1}, {0, -1, 0}};

    const MatrixFilter filter_;
};

class ThresholdFilter : public PixelFilter {
public:
    explicit ThresholdFilter(float);

    void Transform(Image& image) const override;

    static void CheckArgs(const FilterSpec&);
private:
    const float threshold_ = 0;
};


class EdgeDetectionFilter : public CompositeFilter {
public:
    explicit EdgeDetectionFilter(const FilterSpec& spec);

    static void CheckArgs(const FilterSpec&);

private:
    static float ParseThreshold(const FilterSpec& spec);
};

class GaussianBlurFilter : public CompositeFilter {
public:
    explicit GaussianBlurFilter(const FilterSpec& spec);

    static void CheckArgs(const FilterSpec&);

private:
    static float ParseSigma(const FilterSpec& spec);
    static constexpr float MaximalSigma = 1000;
    int32_t Radius(float sigma);
    std::vector<float> Weights(float sigma);
    std::vector<std::vector<float>> Horizontal(float sigma);
    std::vector<std::vector<float>> Vertical(float sigma);
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