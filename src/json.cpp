// JSON parser implementation as per https://datatracker.ietf.org/doc/html/rfc8259
#include <cmath>
#include "json.h"


Value::Value(ValueType type, void* value) {
    this->_type = type;
    this->_value = value;
}

ValueType Value::type() const {
    return this->_type;
}

const void* const Value::value() const {
    return this->_value;
}

const ValuePtr& Value::operator[](std::size_t pos) const {
    // By default, cannot use array index. Erroneous to do so.
    throw;
}


const ValuePtr& Array::operator[](std::size_t pos) const {
    // Arrays can logically have elements accessed by index.
    return ((ValueArray*)(this->value()))->operator[](pos);
}


ValuePtr parse_array(const std::string& string, int& i) {
    i++; // Opening square bracket is known.
    ValueArray* array = new ValueArray;
    bool expecting_comma = false;
    for (; i < string.size(); ++i) {
        char c = string[i];
        if (WHITESPACE.count(c)) {
            continue;
        }
        if (expecting_comma) {
            if (!STRUCTURAL_CHARS.count(c)) {
                throw;
            }
            switch (STRUCTURAL_CHARS[c]) {
                case Structural::value_separator:
                    // Comma as expected.
                    expecting_comma = false;
                    continue;
                case Structural::end_array:
                    // Alternatively, array can end here too.
                    return std::make_unique<Array>(ValueType::array, array);
                default:
                    throw;
            }
        }
        if (STRUCTURAL_CHARS.count(c)) {
            // Allow opening of array, object, or closing or array
            // Only allow comma if expected (not expecting a value).
            switch (STRUCTURAL_CHARS[c]) {
                case Structural::begin_array:
                    array->push_back(parse_array(string, i));
                    break;
                case Structural::begin_object:
                    array->push_back(parse_object(string, i));
                    break;
                case Structural::end_array:
                    // Just had a comma.
                    // Must not end on comma, but array can be empty.
                    if (!array->empty()) {
                        throw;
                    }
                    return std::make_unique<Array>(ValueType::array, array);
                default:
                    // Not expecting any other structural character at this time.
                    throw;
            }
        } else if (c == STRING_QUOTES) {
            array->push_back(parse_string(string, i));
        } else if (c == MINUS_SIGN || isdigit(c)) {
            array->push_back(parse_number(string, i));
        } else {
            // Can only be a literal, or complete nonsense otherwise.
            array->push_back(parse_literal_name(string, i));
        }
        expecting_comma = true;
    }
    // End of string reached without array closure.
    throw;
}


ValuePtr parse_object(const std::string& string, int& i) {
    i++;
}


ValuePtr parse_number(const std::string& string, int& i) {
    // Optional minus sign.
    bool negative = string[i] == MINUS_SIGN;
    if (negative) {
        i++;
    }
    double integer_part = 0;
    double fractional_part = 0;
    int fractional_digits = 0;
    double exponent = 0;
    enum ParsingPart {Integer, Fraction, Exponent};
    ParsingPart parsing_part = Integer;
    bool leading_zero = false;
    bool integer_part_empty = true;
    bool decimal_point_seen = false;
    bool exponent_seen = false;
    bool exponent_empty = true;
    bool exponent_negative = false;
    for (; i < string.size(); ++i) {
        char c = string[i];
        switch (parsing_part) {
            case Integer:
                // Possible chars: ., 0-9 (0 not first), e, E
                if (c == DECIMAL_POINT) {
                    decimal_point_seen = true;
                    parsing_part = Fraction;
                } else if (isdigit(c)) {
                    if (c == '0' && integer_part_empty) {
                        // (Possibly) Insignificant leading 0.
                        leading_zero = true;
                    }
                    if (leading_zero && !integer_part_empty) {
                        // Insignificant leading 0 is illegal.
                        throw;
                    }
                    integer_part = integer_part * 10 + (c - '0');
                    integer_part_empty = false;
                } else if (tolower(c) == EXPONENT) {
                    // Early exponent without fractional part.
                    exponent_seen = true;
                    parsing_part = Exponent;
                } else {
                    // Unrecognised character - assume end of number.
                    goto end_of_number;
                }
                break;
            case Fraction:
                // Possible chars: 0-9, e, E
                if (isdigit(c)) {
                    fractional_digits++;
                    // Nth digit after decimal point is power of 10^-n.
                    fractional_part += (c - '0') * std::pow(10, -fractional_digits);
                } else if (tolower(c) == EXPONENT) {
                    exponent_seen = true;
                    parsing_part = Exponent;
                } else {
                    goto end_of_number;
                }
                break;
            case Exponent:
                // Possible chars: + (first only), - (first only), 0-9
                if (exponent_empty && c == PLUS_SIGN) {
                    // Positive exponent, no effect since it is the default.
                    continue;
                } else if (exponent_empty && c == MINUS_SIGN) {
                    exponent_negative = true;
                } else if (isdigit(c)) {
                    exponent = exponent * 10 + (c - '0');
                    exponent_empty = false;
                } else {
                    goto end_of_number;
                }
                break;
        }
    }
    end_of_number:
    // Decrement so that the current unknown char can be handled in parent scope.
    i--;
    // Integer part must not be empty.
    // Decimal point must be followed by one or more digits.
    // If 'e' is seen, it must be followed by one or more digits.
    if (
        integer_part_empty
        || (decimal_point_seen && fractional_digits == 0)
        || (exponent_seen && exponent_empty)
    ) {
        throw;
    }
    if (exponent_negative) {
        exponent = -exponent;
    }
    double* number = new double((integer_part + fractional_part) * std::pow(10, exponent));
    if (negative) {
        *number = -*number;
    }
    return std::make_unique<Value>(ValueType::number, number);
}


ValuePtr parse_string(const std::string& string, int& i) {
    i++; // Opening double quote.
}


ValuePtr parse_literal_name(const std::string& string, int& i) {
    std::string literal_name = "";
    for (; i < string.size(); ++i) {
        literal_name += string[i];
        if (literal_name.size() > 5) {
            // false is the longest literal, if still going, invalid...
            throw;
        }
        if (LITERAL_NAMES.count(literal_name)) {
            bool* result = nullptr;
            switch (LITERAL_NAMES[literal_name]) {
                case LiteralName::true_:
                    result = new bool(true);
                    break;
                case LiteralName::false_:
                    result = new bool(false);
                    break;
            }
            return std::make_unique<Value>(ValueType::literal_name, result);
        }
    }
    // End of string reached without finding a literal.
    throw;
}


ValuePtr parse_json(const std::string& string) {
    ValuePtr result = nullptr;
    for (int i = 0; i < string.size(); ++i) {
        char c = string[i];
        if (WHITESPACE.count(c)) {
            // Ignore insignificant whitespace.
            continue;
        }
        if (result != nullptr) {
            // Cannot have second part of the toplevel data... error
            throw;
        }
        if (STRUCTURAL_CHARS.count(c)) {
            // In this context, either array/object opening is expected.
            // Anything else is erroneous.
            switch (STRUCTURAL_CHARS[c]) {
                case Structural::begin_array:
                    result = parse_array(string, i);
                    break;
                case Structural::begin_object:
                    result = parse_object(string, i);
                    break;
                default:
                    throw;
            }
            continue;
        }
        if (c == STRING_QUOTES) {
            result = parse_string(string, i);
        } else if (c == MINUS_SIGN || isdigit(c)) {
            result = parse_number(string, i);
        } else {
            // It can be nothing else - either literal name or invalid.
            result = parse_literal_name(string, i);
        }
    }
    if (result == nullptr) {
        // Nothing found but whitespace.
        throw;
    }
    return result;
}