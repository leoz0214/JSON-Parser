# Usage of the JSON Parser
This will serve as a **basic guide** on how to use the JSON parser in a C++ program. For full details, explore the header file and source code, but this will provide a solid overview of the system.

Detailed **example usage** of the parser including example inputs and example handling of the output value can be seen in the `testing/test.cpp` file.

The functions and classes of the library are available through the namespace `json`. `using namespace json;` will introduce all entities into the global namespace, seen in the test files.

Further note, this guide assumes a working knowledge of how JSON works, since you plan to use this JSON parser to handle JSON data in C++. If this is not the case, please research [how JSON works](https://www.json.org/json-en.html) or else this guide will make little sense.

The implemented JSON standard can be found here: https://datatracker.ietf.org/doc/html/rfc8259

## Input
The program can handle two input types: a `std::string` object or a `std::istream` input stream including derived streams such as `std::ifstream` for parsing input JSON files.

If a `std::string` object is provided, the **entire string** is interpreted as a JSON object and parsed accordingly.

If a `std::istream` object is provided, all characters in the **current position up to the end** will be parsed accordingly. After processing, the stream should be at the end of the stream, but if an error occurs during parsing, the stream will be at an arbitrary position.

Call `json::parse`, passing in either type of object and the processing will begin.

## Processing
Below are some key points to note regarding the processing of data by the parser, generally available in more detail in the standards document:
- Insignificant whitespace is **ignored**.
- Objects and arrays can be **empty**.
- Objects can contain **duplicate string keys**, but only the value of the **most recent** instance of the key is kept.
- Numbers can be negative, fractional, and have an exponent, but must not contain insignificant leading 0s. Valid number: -3.14159e+2
- Strings are processed as **UTF-8** (Unicode).
- Strings can contain valid escape characters like \n, and also Unicode escape characters in the Basic Multilingual Plane, represented as \uXXXX, where XXXX is a 4-character case-insensitive hex code e.g. \uaF06
- Valid JSON literals are: true, false and null.

## Output
The return value is of type `json::Value`, which is derived from `std::variant<json::Null, json::Object, json::Array, json::Number, json::String, json::Boolean>`. `std::variant` is a powerful tool in the standard library allowing **different data types** to be stored at runtime, perfect for the dynamic but finite nature of JSON. More information on `std::variant` can be found [here](https://en.cppreference.com/w/cpp/utility/variant). Basic usage includes retrieving a value using `std::get<data_type>(variant);` and checking if a variant holds a particular type using `std::holds_alternative<data_type>(variant);`

Note the use of typedefs for maintainability corresponding to the following:
- `json::Null` - `std::monostate` - empty variant value.
- `json::Object` - `std::unordered_map<std::string, json::Value>` - string keys, heterogeneous values, unordered.
- `json::Array` - `std::vector<json::Value>` - variable length array of heterogeneous values.
- `json::Number` - `double` - can be fractional.
- `json::String` - `std::string`.
- `json::Boolean` - `bool`.

## Exceptions
The parser is reasonably robust, handling erroneous input by throwing exceptions.

All exceptions due to the parse will be of type `json::JsonParseError`, derived from `std::runtime_error`. This means modern C++ exception handling is used (**try-catch**). More information [here](https://en.cppreference.com/w/cpp/language/try_catch).

Exception messages will provide relevant details and the position of the error where appropriate.

## Efficiency
The parser has been optimised on a basic level but is still **not particularly fast** - seek alternative JSON parsers if performance is critical. Nonetheless, time complexity is **linear**, and negligible extra space is used other than storing the resulting JSON value.