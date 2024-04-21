// Corresponding header file for the JSON parser implementation.
#pragma once
#include <cstdint>
#include <istream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>


// Structural characters in JSON.
enum class Structural {
    begin_array, begin_object, end_array, end_object,
    name_separator, value_separator
};

static std::map<char, Structural> STRUCTURAL_CHARS = {
    {'[', Structural::begin_array},
    {'{', Structural::begin_object},
    {']', Structural::end_array},
    {'}', Structural::end_object},
    {':', Structural::name_separator},
    {',', Structural::value_separator}
};

// Spaces, tabs, new lines, carriage return
static std::set<char> WHITESPACE = {0x20, 0x09, 0x0A, 0x0D};

// Strings open and close with DOUBLE quotes.
const char STRING_QUOTES = '"';
// Backslash can be used for escaping.
const char BACKSLASH = '\\';
// Valid escape characters.
static std::map<char, char> ESCAPE_CHARS = {
    {'"', '"'}, // Quotation mark
    {'\\', '\\'}, // Reverse solidus
    {'/', '/'}, // Solidus
    {'b', 0x08}, // Backspace
    {'f', 0x0C}, // Form feed
    {'n', 0x0A}, // Line feed (new line)
    {'r', 0x0D}, // Carriage return
    {'t', 0x09} // Tab
};
// Unicode escape is u followed by 4 hex digits.
const char UNICODE_ESCAPE = 'u';

// Numbers/exponents may start with a minus sign.
const char MINUS_SIGN = '-';
// Exponents may start with a plus sign.
const char PLUS_SIGN = '+';
// Numbers may contain a decimal point to separate the integer/fractional part.
const char DECIMAL_POINT = '.';
// Numbers may contain an exponent part (scientific notation).
const char EXPONENT = 'e';

// Valid literal names.
enum class LiteralName {true_, false_, null};
static std::map<std::string, LiteralName> LITERAL_NAMES = {
    {"true", LiteralName::true_},
    {"false", LiteralName::false_},
    {"null", LiteralName::null}
};


class Value;
// NULL
typedef std::monostate Null;
// The internal representation of a JSON object of keys and values.
typedef std::unordered_map<std::string, Value> Object;
// The internal representation of a JSON array of values.
typedef std::vector<Value> Array;
// Represent JSON numbers in the following type.
typedef double Number;
// Represent JSON strings in the following type.
typedef std::string String;
// Represent JSON booleans in the following type.
typedef bool Boolean;
// Represent JSON value of any type (monostate -> null).
class Value : public std::variant<Null, Object, Array, Number, String, Boolean> {
    using std::variant<Null, Object, Array, Number, String, Boolean>::variant;
};

// Shortcuts instead of verbose std::get<ValueType>
// bool (*ToNull)(const Value&) = std::get<Null>;


// Wraps a string or input stream (support both in the parser)
// so that code repetition can be minimised with minimal performance penalty.
class _DataWrapper {
    protected:
        std::size_t _pos = 0;
    public:
        virtual char get() = 0;
        virtual bool eof() = 0;
        virtual _DataWrapper& operator++() = 0;
        virtual _DataWrapper& operator--() = 0;
        inline std::size_t pos();
        inline void error(const std::string&);
        inline void errorpos(const std::string&, int = -1);
};

// Wraps string to be accessed and parsed.
class _StrWrapper : public _DataWrapper {
    private:
        const std::string* data;
        std::size_t index = 0;
    public:
        _StrWrapper(const std::string&);
        char get() override;
        bool eof() override;
        _StrWrapper& operator++() override;
        _StrWrapper& operator--() override;
};

// Wraps input stream to be accessed and parsed.
class _IstreamWrapper : public _DataWrapper {
    private:
        std::istream* stream;
    public:
        _IstreamWrapper(std::istream&);
        char get() override;
        bool eof() override;
        _IstreamWrapper& operator++() override;
        _IstreamWrapper& operator--() override;
};

// Parses a string object representing the JSON data.
// Will raise an exception if the JSON data is invalid.
Value parse_json(_DataWrapper&);
Value parse_json(const std::string&);
Value parse_json(std::istream&);

// Parses a JSON array.
Value parse_array(_DataWrapper&);

// Parses a JSON object.
Value parse_object(_DataWrapper&);

// Parses a JSON number.
inline Value parse_number(_DataWrapper&);

// Parses a JSON string literal (not the JSON string itself).
inline Value parse_string(_DataWrapper&);

// Parses a JSON literal name (true, false or null).
inline Value parse_literal_name(_DataWrapper&);

// Error object for library.
class JsonParseError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Common error messages.
static std::string INVALID_JSON_DATA = "Invalid JSON data.";