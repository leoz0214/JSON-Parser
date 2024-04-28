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


// Use sensible namespace to keep module contents separate from global namespace.
namespace json {

// Structural characters in JSON.
const char BEGIN_ARRAY = '[';
const char BEGIN_OBJECT = '{';
const char END_ARRAY = ']';
const char END_OBJECT = '}';
const char NAME_SEPARATOR = ':';
const char VALUE_SEPARATOR = ',';

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
// Represent JSON value of any type.
class Value : public std::variant<Null, Object, Array, Number, String, Boolean> {
    using std::variant<Null, Object, Array, Number, String, Boolean>::variant;
};


// Valid literal names.
static std::map<std::string, Value> LITERAL_NAMES = {
    {"true", true},
    {"false", false},
    {"null", Null()}
};


// Error object for library.
class JsonParseError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};


// Wraps a string or input stream (support both in the parser)
// so that code repetition can be minimised with minimal performance penalty.
class _DataWrapper {
    protected:
        std::size_t _pos = 0;
        bool _eof = false;
    public:
        virtual char peek() = 0;
        virtual char get() = 0;
        virtual void operator++() = 0;
        virtual void operator--() = 0;
        inline std::size_t pos();
        inline bool eof();
        inline JsonParseError error(const std::string&);
        inline JsonParseError errorpos(const std::string&, int = -1);
};

// Wraps string to be accessed and parsed.
class _StrWrapper : public _DataWrapper {
    private:
        const std::string* data;
        std::size_t index = 0;
    public:
        _StrWrapper(const std::string&);
        char peek() override;
        char get() override;
        void operator++() override;
        void operator--() override;
};

// Wraps input stream to be accessed and parsed.
class _IstreamWrapper : public _DataWrapper {
    private:
        std::istream* stream;
    public:
        _IstreamWrapper(std::istream&);
        char peek() override;
        char get() override;
        void operator++() override;
        void operator--() override;
};

// Parses a string object representing the JSON data.
// Will raise an exception if the JSON data is invalid.
Value parse(_DataWrapper&);
Value parse(const std::string&);
Value parse(std::istream&);

// Parses a JSON array.
Value _parse_array(_DataWrapper&);

// Parses a JSON object.
Value _parse_object(_DataWrapper&);

// Parses a JSON number.
inline Value _parse_number(_DataWrapper&);

// Parses a JSON string literal (not the JSON string itself).
inline Value _parse_string(_DataWrapper&);

// Parses a JSON literal name (true, false or null).
inline Value _parse_literal_name(_DataWrapper&);

static std::string INVALID_JSON_DATA = "Invalid JSON data.";

}