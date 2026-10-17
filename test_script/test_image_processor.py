#!/usr/bin/python3

from collections import namedtuple
from functools import reduce
from PIL import Image, ImageChops, UnidentifiedImageError
import math
import operator
import os
import subprocess
import sys
import tempfile


def calc_images_distance(image_path1, image_path2):
    with Image.open(image_path1) as image1, Image.open(image_path2) as image2:
        histogram = ImageChops.difference(image1, image2).histogram()

        return math.sqrt(reduce(operator.add, map(lambda h, i: h * (i ** 2), histogram, range(256))) / (
                float(image1.size[0]) * image1.size[1]))


class ImageProcessorTester:
    TestCase = namedtuple("TestCase", ["name", "input", "args", "eps"])

    class TestCaseFailedException(Exception):
        pass

    def __init__(self, image_processor_executable):
        self.image_processor_executable = image_processor_executable

    @staticmethod
    def fail_test_case(input, name, message):
        print("FAIL [{input}_{name}] {message}".format(input=input, name=name, message=message))
        raise ImageProcessorTester.TestCaseFailedException()

    @staticmethod
    def succeed_test_case(input, name):
        print("OK   [{input}_{name}]".format(input=input, name=name))

    def run_tests(self):
        filter_test_cases = {
            "crop": [
                ImageProcessorTester.TestCase(input="flag", name="crop", args=["-crop", "50", "50"], eps=0.0),
            ],
            "edge": [
                ImageProcessorTester.TestCase(input="flag", name="edge", args=["-edge", "0.1"], eps=1.0),
                ImageProcessorTester.TestCase(input="flag", name="edge_edge", args=["-edge", "0.1", "-edge", "0.5"],
                                              eps=1.0),
            ],
            "gs": [
                ImageProcessorTester.TestCase(input="flag", name="gs", args=["-gs"], eps=1.0),
            ],
            "neg": [
                ImageProcessorTester.TestCase(input="flag", name="neg", args=["-neg"], eps=1.0),
            ],
            "sharp": [
                ImageProcessorTester.TestCase(input="flag", name="sharp", args=["-sharp"], eps=1.0)
            ],
            "blur": [

            ],
        }
        failed_filters = []

        for filter_name, test_cases in filter_test_cases.items():
            try:
                for test_case in test_cases:
                    self.run_test_case(test_case)
            except ImageProcessorTester.TestCaseFailedException:
                failed_filters.append(filter_name)

        if failed_filters:
            print("-----\nFAILED {count} FILTERS: {filters}\n-----".format(
                count=len(failed_filters),
                filters=", ".join(failed_filters)))
            return False
        else:
            print("-----\nALL FILTERS OK\n-----")
            return True

    def run_tests_without_checking_output(self):
        cases = [
            ("без фильтров", [], 0),
            ("размытие", ["-blur", "1"], 0),
            ("цепочка фильтров", ["-gs", "-neg", "-blur", "0.5"], 0),
            ("crop mini", ["-crop", "1", "1"], 0),
            ("crop одна колонка", ["-crop", "1", "10000"], 0),
            ("crop одна строка", ["-crop", "10000", "1"], 0),
            ("большой crop", ["-crop", "10000", "10000"], 0),
            ("crop gigavalue", ["-crop", "1000000000", "1000000000"], 0),
            ("crop cropa", ["-crop", "1", "1", "-crop", "100", "100"], 0),
            ("sharp на микро", ["-crop", "1", "1", "-sharp"], 0),
            ("edge на микро", ["-crop", "1", "1", "-edge", "0"], 0),
            ("blur на микро", ["-crop", "1", "1", "-blur", "1"], 0),
            ("маленькая sigma", ["-blur", "0.000001"], 0),
            ("максимальная sigma", ["-crop", "1", "1", "-blur", "1000"], 0),
            ("за границей sigma", ["-crop", "1", "1", "-blur", "1001"], 1),
            ("sigma в нестардартной записи", ["-blur", "1e-3"], 0),
            ("edge нижняя граница", ["-edge", "0"], 0),
            ("edge верхняя граница", ["-edge", "1"], 0),
            ("voronoi одна точка", ["-voronoi", "1"], 0),
            ("voronoi на 1x1", ["-crop", "1", "1", "-voronoi", "1"], 0),
            ("voronoi больше точек чем пикселей", ["-voronoi", "1000"], 0),
            ("неизвестный фильтр", ["-unknown"], 1),
            ("одинокий дефис", ["-"], 1),
            ("двойной дефис", ["--"], 1),
            ("минус забыл", ["gs"], 1),
            ("пустой аргумент", [""], 1),
            ("откуда взялось то", ["123"], 1),
            ("неверный регистр", ["-GS"], 1),
            ("crop мало аргументов", ["-crop", "1", "-gs"], 1),
            ("blur мало аргументов", ["-blur", "-neg"], 1),
            ("рандом ошибка", ["-gs", "-neg", "-blur", "abc"], 1),
        ]

        for filter_name in ("-gs", "-neg", "-sharp"):
            cases.append((f"{filter_name}: лишний параметр", [filter_name, "1"], 1))

        for filter_name, valid_params in (
                ("-crop", ["1", "1"]),
                ("-blur", ["1"]),
                ("-edge", ["0.1"]),
                ("-voronoi", ["1"])
                ):
            cases.append((f"{filter_name}: нет параметров", [filter_name], 1))
            cases.append((f"{filter_name}: лишний параметр", [filter_name] + valid_params + ["2"], 1))

        for value in ("0", "-1", "-2147483648", "2147483648", "999999999999999999999999", "1.5", "1e2", "abc", "1px", ""):
            cases.append((f"crop: ширина {value}", ["-crop", value, "1"], 1))
            cases.append((f"crop: высота {value}", ["-crop", "1", value], 1))

        for value in ("0", "-0", "-0.1", "1000.1", "1e30"):
            cases.append((f"blur: sigma вне диапазона {value}", ["-blur", value], 1))

        for value in ("nan", "NaN", "inf", "-inf", "1e999", "1e-999", "abc", "0.5abc", "0,5", ".", ""):
            for filter_name in ("-blur", "-edge"):
                cases.append((f"{filter_name}: невалидное число {value}", [filter_name, value], 1))

        for value in ("0", "-1", "6767676767", "1.5", "abc", "1point", ""):
            cases.append((f"voronoi: невалидное число точек {value}", ["-voronoi", value], 1))

        success = True

        for name, args, expected_exit_code in cases:
            with tempfile.TemporaryDirectory() as directory:
                output = os.path.join(directory, "output.bmp")

                try:
                    result = subprocess.run(
                        [
                            self.image_processor_executable,
                            "test_script/data/flag.bmp",
                            output,
                        ] + args,
                        capture_output=True,
                        text=True,
                        errors="replace",
                        timeout=10,
                    )
                    if result.returncode != expected_exit_code:
                        print(f"FAIL [{name}] expected exit {expected_exit_code}, got {result.returncode}")
                        print((result.stdout + result.stderr).strip())
                        success = False
                    else:
                        print(f"OK   [{name}]")
                except (subprocess.TimeoutExpired, OSError) as error:
                    print(f"FAIL [{name}] {error}")
                    success = False
                    
        return success

    def run_test_case(self, test_case):
        try:
            input_file_name = "{input}.bmp".format(input=test_case.input)
            output_file_name = "{input}_{name}.bmp".format(input=test_case.input, name=test_case.name)
            input_file = os.path.join("test_script", "data", input_file_name)
            expected_output_file = os.path.join("test_script", "data", output_file_name)

            with tempfile.NamedTemporaryFile(suffix=".bmp") as output_file:
                subprocess.check_call([self.image_processor_executable, input_file, output_file.name] + test_case.args,
                                      timeout=180)

                images_distance = calc_images_distance(expected_output_file, output_file.name)
                if images_distance > test_case.eps:
                    self.fail_test_case(test_case.input, test_case.name,
                                        "output image differs from expected with rms diff {diff}".format(
                                            diff=images_distance))

            self.succeed_test_case(test_case.input, test_case.name)
        except subprocess.CalledProcessError:
            self.fail_test_case(test_case.input, test_case.name, "image_processor finished with non-zero exit code")
        except subprocess.TimeoutExpired:
            self.fail_test_case(test_case.input, test_case.name, "timeout")
        except FileNotFoundError:
            self.fail_test_case(test_case.input, test_case.name, "output file not found")
        except UnidentifiedImageError:
            self.fail_test_case(test_case.input, test_case.name, "output file is corrupt")


if __name__ == "__main__":
    tester = ImageProcessorTester(image_processor_executable=sys.argv[1])

    print(
        "Running image_processor tests\nExecutable: {executable}".format(executable=tester.image_processor_executable))

    image_tests_ok = tester.run_tests()
    cli_tests_ok = tester.run_tests_without_checking_output()
    if not image_tests_ok or not cli_tests_ok:
        sys.exit(1)
