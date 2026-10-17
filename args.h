#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

struct FilterSpec {
    std::string_view name;
    std::vector<std::string_view> params;
};

struct Args {
    std::string_view input;
    std::string_view output;
    std::vector<FilterSpec> filters;
};

Args ParseArgs(size_t, char**);