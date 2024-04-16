// Corresponding header file for the JSON parser implementation.
#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
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

// A JSON value MUST be an object, array, number, or string, or true/false/null
enum class ValueType {object, array, number, string, literal_name};

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
// Wrap a dynamic JSON value in a memory-safe unique pointer.
typedef std::unique_ptr<Value> ValuePtr;
// Represents a JSON value. Will be polymorphic such that the type of
// the value can be deduced at runtime, not compile-time.
class Value {
    private:
        ValueType _type;
        const void* _value;
    public:
        Value() = default;
        Value(ValueType, void*);
        ValueType type() const;
        const void* const value() const;
        virtual const ValuePtr& operator[](std::size_t) const;
        virtual const ValuePtr& operator[](const std::string&) const;
};

// Represents a JSON array, storing an ordered sequence of
// heterogeneous values (not a traditional array!).
class Array : public Value {
    using Value::Value;
    const ValuePtr& operator[](std::size_t) const override;
};

// Represents a JSON object, storing an unordered associative
// array of unique string keys and heterogeneous values.
class Object : public Value {
    using Value::Value;
    const ValuePtr& operator[](const std::string&) const override;
};

// The internal representation of a JSON array of values.
typedef std::vector<ValuePtr> ValueArray;
// The internal representation of a JSON object of keys and values.
typedef std::unordered_map<std::string, ValuePtr> ValueObject;

// Parses a string object representing the JSON data.
// Will raise an exception if the JSON data is invalid.
ValuePtr parse_json(const std::string&);

// Parses a JSON array.
ValuePtr parse_array(const std::string&, int&);

// Parses a JSON object.
ValuePtr parse_object(const std::string&, int&);

// Parses a JSON number.
ValuePtr parse_number(const std::string&, int&);

// Parses a JSON string literal (not the JSON string itself).
ValuePtr parse_string(const std::string&, int&);

// Parses a JSON literal name (true, false or null).
ValuePtr parse_literal_name(const std::string&, int&);