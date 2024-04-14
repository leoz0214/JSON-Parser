// Corresponding header file for the JSON parser implementation.
#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
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
};

// Represents a JSON array, storing an ordered sequence of
// heterogeneous values (not a traditional array!).
class Array : public Value {
    using Value::Value;
    const ValuePtr& operator[](std::size_t) const override;
};

// The internal representation of a JSON array of values.
typedef std::vector<ValuePtr> ValueArray;

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