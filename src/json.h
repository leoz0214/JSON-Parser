// Corresponding header file for the JSON parser implementation.
#pragma once
#include <map>
#include <set>
#include <string>


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

// Numbers may start with a minus sign.
const char MINUS_SIGN = '-';

// Literal name to actual value (pointer).
static std::map<std::string, void*> LITERAL_NAME_MAP = {
    {"true", new bool {true}},
    {"false", new bool {false}},
    {"null", nullptr}
};

// Represents a JSON value. Will be polymorphic such that the type of
// the value can be deduced at runtime, not compile-time.
class Value {
    private:
        ValueType _type;
        void* _value;
    public:
        Value() = default;
        Value(ValueType, void*);
        ValueType type() const;
        void* const value() const;
};

// Parses a string object representing the JSON data.
// Will raise an exception if the JSON data is invalid.
Value parse_json(const std::string&);

// Parses a JSON array.
Value parse_array(const std::string&, int&);

// Parses a JSON object.
Value parse_object(const std::string&, int&);

// Parses a JSON number.
Value parse_number(const std::string&, int&);

// Parses a JSON string literal (not the JSON string itself).
Value parse_string(const std::string&, int&);

// Parses a JSON literal name (true, false or null).
Value parse_literal_name(const std::string&, int&);