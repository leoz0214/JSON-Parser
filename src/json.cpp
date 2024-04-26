// JSON parser implementation as per https://datatracker.ietf.org/doc/html/rfc8259
#include <cmath>
#include "json.h"


namespace json {

std::size_t  _DataWrapper::pos() {
    return this->_pos;
}

bool _DataWrapper::eof() {
    return this->_eof;
}

JsonParseError _DataWrapper::error(const std::string& what) {
    return JsonParseError(what);
}

JsonParseError _DataWrapper::errorpos(const std::string& what, int pos) {
    if (pos == -1) {
        pos = this->pos();
    }
    std::string message = "Error at position " + std::to_string(pos) + ": " + what;
    return this->error(message);
}


_StrWrapper::_StrWrapper(const std::string& data) {
    this->data = &data;
}

char _StrWrapper::peek() {
    char c = this->data->operator[](this->_pos);
    this->_eof = this->_pos == this->data->size();
    return c;
}

char _StrWrapper::get() {
    char c = this->data->operator[](this->_pos);
    this->_eof = this->_pos == this->data->size();
    this->_pos++;
    return c;
}

void _StrWrapper::operator++() {
    this->_pos++;
    this->_eof = this->_pos == this->data->size();
}

void _StrWrapper::operator--() {
    this->_pos--;
    this->_eof = false;
}


_IstreamWrapper::_IstreamWrapper(std::istream& istream) {
    this->stream = &istream;
}

char _IstreamWrapper::peek() {
    char c = this->stream->peek();
    this->_eof = c == EOF;
    return c;
}

char _IstreamWrapper::get() {
    this->_pos++;
    char c = this->stream->get();
    this->_eof = c == EOF;
    return c;
}

void _IstreamWrapper::operator++() {
    char c = this->stream->get();
    this->_pos++;
    this->_eof = c == EOF;
}

void _IstreamWrapper::operator--() {
    this->stream->unget();
    this->_pos--;
    this->_eof = false;
}


// Parse any JSON value. It could be a number, string, literal, array or object.
inline Value _parse_value(_DataWrapper& data) {
    char c = data.peek();
    switch (c) {
        case STRING_QUOTES:
            return _parse_string(data);
        case BEGIN_ARRAY:
            return _parse_array(data);
        case BEGIN_OBJECT:
            return _parse_object(data);
        default:
            if (c == MINUS_SIGN || isdigit(c)) {
                return _parse_number(data);
            }
            // It can be nothing else - either literal name or invalid.
            return _parse_literal_name(data);
    }
}


Value _parse_array(_DataWrapper& data) {
    ++data; // Opening square bracket is known.
    Array array;
    bool expecting_comma = false;
    while (true) {
        char c = data.peek();
        if (data.eof()) {
            break;
        }
        if (WHITESPACE.count(c)) {
            ++data;
            continue;
        }
        if (expecting_comma) {
            switch (c) {
                case VALUE_SEPARATOR:
                    // Comma as expected.
                    expecting_comma = false;
                    ++data;
                    continue;
                case END_ARRAY:
                    // Alternatively, array can end here too.
                    ++data;
                    return array;
                default:
                    throw data.errorpos("Expected comma.");
            }
        }
        if (c == END_ARRAY) {
            // Just had a comma. Must not end on comma, but array can be empty.
            if (!array.empty()) {
                throw data.errorpos("Expected value.");
            }
            ++data;
            return array;
        }
        array.push_back(_parse_value(data));
        expecting_comma = true;
    }
    throw data.errorpos("Array not closed.");
}


Value _parse_object(_DataWrapper& data) {
    ++data; // Opening curly bracket is known.
    Object object;
    enum ObjectParsingPart {Name, Colon, Value, Comma};
    int parsing_part = Name;
    String key;
    while (true) {
        char c = data.peek();
        if (data.eof()) {
            break;
        }
        if (WHITESPACE.count(c)) {
            ++data;
            continue;
        }
        switch (parsing_part) {
            case ObjectParsingPart::Name:
                if (c == STRING_QUOTES) {
                    // Deduce string key ready to be paired to corresponding value.
                    // Retain it in order to map it to the corresponding value to come.
                    // Allow duplicate keys (leniency) but will overwrite existing value.
                    key = std::get<String>(_parse_string(data));
                } else if (object.empty() && c == END_OBJECT) {
                    // End of object successfully reached.
                    ++data;
                    return object;
                } else {
                    throw data.errorpos("Expected string literal as object key.");
                }
                break;
            case ObjectParsingPart::Colon:
                if (c != NAME_SEPARATOR) {
                    throw data.errorpos("Expected colon.");
                }
                ++data;
                break;
            case ObjectParsingPart::Value:
                object[key] = _parse_value(data);
                break;
            case ObjectParsingPart::Comma:
                switch (c) {
                    case VALUE_SEPARATOR:
                        break;
                    case END_OBJECT:
                        // Object ends here instead of another comma.
                        ++data;
                        return object;
                    default:
                        throw data.errorpos("Expected comma.");
                }
                ++data;
                break;
        }
        // To the next parsing part, cycling back to Name after Comma.
        parsing_part = (parsing_part + 1) % 4;
    }
    throw data.errorpos("Object not closed.");
}


enum class NumberParsingPart {Integer, Fraction, Exponent};
Value _parse_number(_DataWrapper& data) {
    // Optional minus sign.
    std::size_t start_pos = data.pos();
    bool negative = data.peek() == MINUS_SIGN;
    if (negative) {
        ++data;
    }
    double integer_part = 0;
    double fractional_part = 0;
    int fractional_digits = 0;
    double exponent = 0;
    NumberParsingPart parsing_part = NumberParsingPart::Integer;
    bool leading_zero = false;
    bool integer_part_empty = true;
    bool decimal_point_seen = false;
    bool exponent_seen = false;
    bool exponent_empty = true;
    bool exponent_sign_seen = false;
    bool exponent_negative = false;
    while (true) {
        char c = data.get();
        if (data.eof()) {
            break;
        }
        switch (parsing_part) {
            case NumberParsingPart::Integer:
                // Possible chars: ., 0-9 (no insignificant 0s), e, E
                if (c == DECIMAL_POINT) {
                    decimal_point_seen = true;
                    parsing_part = NumberParsingPart::Fraction;
                } else if (isdigit(c)) {
                    if (c == '0' && integer_part_empty) {
                        // (Possibly) Insignificant leading 0.
                        leading_zero = true;
                    }
                    if (leading_zero && !integer_part_empty) {
                        throw data.errorpos("Insignificant leading 0s disallowed.", start_pos);
                    }
                    integer_part = integer_part * 10 + (c - '0');
                    integer_part_empty = false;
                } else if (tolower(c) == EXPONENT) {
                    // Early exponent without fractional part.
                    exponent_seen = true;
                    parsing_part = NumberParsingPart::Exponent;
                } else {
                    // Unrecognised character - assume end of number.
                    goto end_of_number;
                }
                break;
            case NumberParsingPart::Fraction:
                // Possible chars: 0-9, e, E
                if (isdigit(c)) {
                    fractional_digits++;
                    // Nth digit after decimal point is power of 10^-n.
                    fractional_part += (c - '0') * std::pow(10, -fractional_digits);
                } else if (tolower(c) == EXPONENT) {
                    exponent_seen = true;
                    parsing_part = NumberParsingPart::Exponent;
                } else {
                    goto end_of_number;
                }
                break;
            case NumberParsingPart::Exponent:
                // Possible chars: + (first only), - (first only), 0-9
                if (exponent_empty && c == PLUS_SIGN && !exponent_sign_seen) {
                    // Positive exponent, no effect since it is the default.
                    exponent_sign_seen = true;
                    continue;
                } else if (exponent_empty && c == MINUS_SIGN && !exponent_sign_seen) {
                    exponent_negative = true;
                    exponent_sign_seen = true;
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
    --data;
    // Integer part must not be empty.
    // Decimal point must be followed by one or more digits.
    // If 'e' is seen, it must be followed by one or more digits.
    if (
        integer_part_empty
        || (decimal_point_seen && fractional_digits == 0)
        || (exponent_seen && exponent_empty)
    ) {
        throw data.errorpos("Invalid number literal.", start_pos);
    }
    if (exponent_negative) {
        exponent = -exponent;
    }
    Number number = (integer_part + fractional_part) * std::pow(10, exponent);
    if (negative) {
        number = -number;
    }
    return number;
}


// Fill UTF-8 byte with bit values (not the hard-coded constant bits).
// Mutates the byte (fills it in) and code point.
inline void fill_utf8_byte(unsigned char& byte, int& code_point, int count) {
    for (int i = 0; i < count; ++i) {
        // Sets ith least significant bit to current LSB of code point.
        byte |= (code_point & 1) << i;
        code_point >>= 1;
    }
}


enum class StringParsingPart {normal, escape, unicode_escape};
Value _parse_string(_DataWrapper& data) {
    ++data; // Opening double quote.
    String result;
    int unicode_code_point = 0;
    int unicode_escape_chars = 0;
    StringParsingPart parsing_part = StringParsingPart::normal;
    while (true) {
        char c = data.get();
        if (data.eof()) {
            break;
        }
        switch (parsing_part) {
            case StringParsingPart::normal:
                // Handle normal character.
                switch (c) {
                    case STRING_QUOTES:
                        // The string has been closed.
                        return result;
                    case BACKSLASH:
                        // Start of an escaped character.
                        parsing_part = StringParsingPart::escape;
                        break;
                    default:
                        // Just a normal character - append.
                        result.push_back(c);
                }
                break;
            case StringParsingPart::escape:
                // Handle escape character without yet knowing what is to come.
                if (ESCAPE_CHARS.count(c)) {
                    parsing_part = StringParsingPart::normal;
                    result.push_back(ESCAPE_CHARS[c]);
                } else if (c == UNICODE_ESCAPE) {
                    parsing_part = StringParsingPart::unicode_escape;
                } else {
                    throw data.errorpos("Invalid escape character.", data.pos() - 1);
                }
                break;
            case StringParsingPart::unicode_escape:
                // Handles Unicode escape character (\uXXXX).
                if (!isxdigit(c)) {
                    throw data.errorpos("Invalid hex character in Unicode escape.", data.pos() - 1);
                }
                c = toupper(c);
                int decimal_value = (c >= 'A') ? (c - 'A' + 10) : (c - '0');
                unicode_code_point = (unicode_code_point * 16) + decimal_value;
                unicode_escape_chars++;
                if (unicode_escape_chars == 4) {
                    // Done with the Unicode escape character.
                    // Convert into sequence of UTF-8 bytes.
                    // Only need to consider 1-3 bytes since range
                    // of Basic Multilingual Plane is [0, 0xFFFF].
                    if (unicode_code_point <= 0x007F) {
                        // Just normal ASCII character - no special stuff.
                        result.push_back(unicode_code_point);
                    } else if (unicode_code_point <= 0x07FF) {
                        // Two bytes: 110xxxxx 10xxxxxx
                        unsigned char byte1 = 0;
                        unsigned char byte2 = 0;
                        fill_utf8_byte(byte2, unicode_code_point, 6);
                        fill_utf8_byte(byte1, unicode_code_point, 5);
                        byte1 |= 0b11000000;
                        byte2 |= 0b10000000;
                        result.append({char(byte1), char(byte2)});
                    } else {
                        // Three bytes: 1110xxxx 10xxxxxx 10xxxxxx
                        unsigned char byte1 = 0;
                        unsigned char byte2 = 0;
                        unsigned char byte3 = 0;
                        fill_utf8_byte(byte3, unicode_code_point, 6);
                        fill_utf8_byte(byte2, unicode_code_point, 6);
                        fill_utf8_byte(byte1, unicode_code_point, 4);
                        byte1 |= 0b11100000;
                        byte2 |= 0b10000000;
                        byte3 |= 0b10000000;
                        result.append({char(byte1), char(byte2), char(byte3)});
                    }
                    parsing_part = StringParsingPart::normal;
                    unicode_code_point = 0;
                    unicode_escape_chars = 0;
                }
        }
    }
    // End of JSON without closing double quote - erroneous.
    throw data.errorpos("Unterminated string literal.", data.pos() - 1);
}


Value _parse_literal_name(_DataWrapper& data) {
    std::string literal_name = "";
    std::size_t start_pos = data.pos();
    while (true) {
        char c = data.get();
        if (data.eof()) {
            break;
        }
        literal_name += c;
        if (literal_name.size() > 5) {
            // false is the longest literal, if still going, invalid...
            break;
        }
        if (LITERAL_NAMES.count(literal_name)) {
            return LITERAL_NAMES[literal_name];
        }
    }
    throw data.errorpos("Invalid literal.", start_pos);
}


Value parse(_DataWrapper& data) {
    Value result;
    bool parsed = false;
    while (true) {
        char c = data.peek();
        if (data.eof()) {
            break;
        }
        if (WHITESPACE.count(c)) {
            // Ignore insignificant whitespace.
            ++data;
            continue;
        }
        if (parsed) {
            // Cannot have second part of the toplevel data... error
            throw data.error(INVALID_JSON_DATA);
        }
        result = _parse_value(data);
        parsed = true;
    }
    if (!parsed) {
        // Nothing found but whitespace.
        throw data.error(INVALID_JSON_DATA);
    }
    return result;
}


Value parse(const std::string& string) {
    _StrWrapper wrapper(string);
    return parse(wrapper);
}


Value parse(std::istream& istream) {
    _IstreamWrapper wrapper(istream);
    return parse(wrapper);
}

}