#include "factory.h"
#include <memory>
#include "args.h"
#include "filters.h"
#include "map"
#include "exceptions.h"

namespace {
using Creator = std::unique_ptr<Filter> (*)(const FilterSpec& spec);
using Checker = void (*)(const FilterSpec&);

template <typename T>
std::unique_ptr<Filter> Make(const FilterSpec& spec) {
    return std::make_unique<T>(spec);
}

const std::map<std::string_view, Creator> K_CREATOR = {
    {"crop", Make<CropFilter>},        {"gs", Make<GrayscaleFilter>},       {"neg", Make<NegativeFilter>},
    {"sharp", Make<SharpeningFilter>}, {"edge", Make<EdgeDetectionFilter>}, {"blur", Make<GaussianBlurFilter>},
    {"voronoi", Make<VoronoiFilter>}};

const std::map<std::string_view, Checker> K_CHECKER = {
    {"crop", &CropFilter::CheckArgs},          {"gs", &GrayscaleFilter::CheckArgs},
    {"neg", &NegativeFilter::CheckArgs},       {"sharp", &SharpeningFilter::CheckArgs},
    {"edge", &EdgeDetectionFilter::CheckArgs}, {"blur", &GaussianBlurFilter::CheckArgs},
    {"voronoi", &VoronoiFilter::CheckArgs}};
}  // namespace

void CheckArguments(const FilterSpec &args) {
    const auto it = K_CHECKER.find(args.name);
    if (it == K_CHECKER.end()) {
        throw UnknownFilterError("unknown filter");
    }

    it->second(args);
}

std::unique_ptr<Filter> CreateFilter(const FilterSpec &args) {
    const auto it = K_CREATOR.find(args.name);
    if (it == K_CREATOR.end()) {
        throw UnknownFilterError("unknown filter");
    }
    return it->second(args);
}
