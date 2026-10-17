#include "args.h"
#include "exceptions.h"
#include <cstddef>
#include <iostream>

void ShowHelp() {
    std::cout << R"(Обработка изображений BMP

Использование:
  image_processor INPUT.bmp OUTPUT.bmp [фильтры...]

Формат: 24-битный BMP без сжатия.

Фильтры:
  -crop WIDTH HEIGHT  Обрезать верхнюю левую часть изображения.
                      WIDTH и HEIGHT — положительные целые числа.
  -gs                 Перевести в оттенки серого.
  -neg                Применить негатив.
  -sharp              Повысить резкость.
  -edge THRESHOLD      Выделить границы с заданным порогом.
  -blur SIGMA          Применить гауссово размытие: 0 < SIGMA <= 1000.
  -voronoi POINTS      Создать мозаику Вороного.
                      POINTS — положительное целое число.

Фильтры применяются в указанном порядке.
Без фильтров изображение сохраняется без изменений.

Пример:
  image_processor input.bmp output.bmp -crop 800 600 -gs -blur 0.5
)";
}

bool IsFilterStart(std::string_view arg) {
    if (!arg.starts_with('-')) {
        return false;
    }

    if (arg.size() > 1) {
        const char next = arg[1];
        if ((next >= '0' && next <= '9') || next == '.') {
            return false;
        }
    }

    return true;
}

Args ParseArgs(size_t argc, char **argv) {
    const std::vector<std::string_view> params(argv, argv + argc);

    Args result;
    if (argc == 1) {
        ShowHelp();
        exit(0);
    }
    if (argc < 3) {
        throw NotEnoughArgumentsError("not enough args");
    }

    result.input = params[1];
    result.output = params[2];

    for (size_t i = 3; i < argc;) {
        if (params[i].size() < 2 || !params[i].starts_with('-')) {
            throw UnexceptedArgsError("unexcepted arg");
        }

        FilterSpec filter;
        filter.name = params[i].substr(1);

        size_t j = i + 1;
        while (j < argc && !IsFilterStart(params[j])) {
            filter.params.push_back(params[j]);
            ++j;
        }

        result.filters.push_back(filter);
        i = j;
    }

    return result;
}