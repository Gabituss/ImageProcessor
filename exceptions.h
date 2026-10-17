#pragma once

#include <stdexcept>

class ImageProcessorError : public ::std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class BmpError : public ImageProcessorError {
public:
    using ImageProcessorError::ImageProcessorError;
};

class BmpOpenError : public BmpError {
public:
    using BmpError::BmpError;
};

class BmpReadingError : public BmpError {
public:
    using BmpError::BmpError;
};

class BmpWritingError : public BmpError {
public:
    using BmpError::BmpError;
};

class BmpProvidedNotABmpError : public BmpError {
public:
    using BmpError::BmpError;
};

class ParseArgsError : public ImageProcessorError {
public:
    using ImageProcessorError::ImageProcessorError;
};

class NotEnoughArgumentsError : public ParseArgsError {
public:
    using ParseArgsError::ParseArgsError;
};

class TooManyArgumentsError : public ParseArgsError {
public:
    using ParseArgsError::ParseArgsError;
};

class UnexceptedArgsError : public ParseArgsError {
public:
    using ParseArgsError::ParseArgsError;
};

class UnknownFilterError : public ParseArgsError {
public:
    using ParseArgsError::ParseArgsError;
};

class ValueOutOfRange : public ParseArgsError {
public:
    using ParseArgsError::ParseArgsError;
};

class FilterInitError : public ImageProcessorError {
public:
    using ImageProcessorError::ImageProcessorError;
};

class ArgumentConversionError : public ImageProcessorError {
public:
    using ImageProcessorError::ImageProcessorError;
};