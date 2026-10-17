#pragma once

#include <memory>
#include "filters.h"
#include "args.h"

void CheckArguments(const FilterSpec &args);
std::unique_ptr<Filter> CreateFilter(const FilterSpec &args);