#include <memory>
#include <vector>
#include "bmp.h"
#include "exceptions.h"
#include "factory.h"
#include "image.h"
#include "args.h"
#include "filters.h"
#include <iostream>

int main(int argc, char **argv) {
    try {
        Args args = ParseArgs(argc, argv);

        Image image = bmp::Read(args.input);

        for (const auto &filter : args.filters) {
            CheckArguments(filter);
        }

        std::vector<std::unique_ptr<Filter>> filters;
        for (const auto &filter : args.filters) {
            filters.push_back(CreateFilter(filter));
        }

        for (const auto &filter : filters) {
            filter->Apply(image);
        }

        bmp::Write(image, args.output);
    } catch (const ImageProcessorError &e) {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
