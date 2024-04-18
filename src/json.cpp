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

const ValuePtr& Value::operator[](const std::string& key) const {
    // Cannot use string key operator by default.
    throw;
}

Value::~Value() {
    if (this->value() == nullptr) {
        return;
    }
    switch (this->type()) {
        case ValueType::object:
            delete (ValueObject*)(this->value());
            break;
        case ValueType::array:
            delete (ValueArray*)(this->value());
            break;
        case ValueType::number:
            delete (double*)(this->value());
            break;
        case ValueType::string:
            delete (std::string*)(this->value());
            break;
        case ValueType::literal_name:
            delete (bool*)(this->value());
            break;
    }
}


const ValuePtr& Array::operator[](std::size_t pos) const {
    // Arrays can logically have elements accessed by index.
    return ((ValueArray*)(this->value()))->operator[](pos);
}


const ValuePtr& Object::operator[](const std::string& key) const {
    // Objects can logically have their values accessed by key.
    return ((ValueObject*)(this->value()))->operator[](key);
}


_StrWrapper::_StrWrapper(const std::string& data) {
    this->data = &data;
}

char _StrWrapper::get() {
    if (this->eof()) {
        throw;
    }
    return this->data->at(this->index);
}

bool _StrWrapper::eof() {
    return this->index == this->data->size();
}

_StrWrapper& _StrWrapper::operator++() {
    if (this->eof()) {
        throw;
    }
    this->index++;
    return *this;
}

_StrWrapper& _StrWrapper::operator--() {
    if (this->index == 0) {
        throw;
    }
    this->index--;
    return *this;
}


_IstreamWrapper::_IstreamWrapper(std::istream& istream) {
    this->stream = &istream;
    this->initial_pos = istream.tellg();
    this->pos = this->initial_pos;
}

char _IstreamWrapper::get() {
    if (this->eof()) {
        throw;
    }
    return this->stream->peek();
}

bool _IstreamWrapper::eof() {
    return this->stream->peek() == EOF;
}

_IstreamWrapper& _IstreamWrapper::operator++() {
    if (this->eof()) {
        throw;
    }
    pos++;
    this->stream->get();
    return *this;
}

_IstreamWrapper& _IstreamWrapper::operator--() {
    if (this->pos == this->initial_pos) {
        throw;
    }
    pos--;
    this->stream->unget();
    return *this;
}


// Parse any JSON value. It could be a number, string, literal, array or object.
inline ValuePtr parse_value(_DataWrapper& data) {
    char c = data.get();
    if (STRUCTURAL_CHARS.count(c)) {
        // In this context, either array/object opening is expected.
        // Anything else is erroneous.
        switch (STRUCTURAL_CHARS[c]) {
            case Structural::begin_array:
                return parse_array(data);
            case Structural::begin_object:
                return parse_object(data);
            default:
                throw;
        }
    }
    if (c == STRING_QUOTES) {
        return parse_string(data);
    } else if (c == MINUS_SIGN || isdigit(c)) {
        return parse_number(data);
    }
    // It can be nothing else - either literal name or invalid.
    return parse_literal_name(data);
}


ValuePtr parse_array(_DataWrapper& data) {
    ++data; // Opening square bracket is known.
    ValueArray* array = new ValueArray;
    bool expecting_comma = false;
    for (; !data.eof(); ++data) {
        char c = data.get();
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
        if (STRUCTURAL_CHARS.count(c) && STRUCTURAL_CHARS[c] == Structural::end_array) {
            // Just had a comma.
            // Must not end on comma, but array can be empty.
            if (!array->empty()) {
                throw;
            }
            return std::make_unique<Array>(ValueType::array, array);
        }
        array->push_back(parse_value(data));
        expecting_comma = true;
    }
    // End of string reached without array closure.
    throw;
}


ValuePtr parse_object(_DataWrapper& data) {
    ++data; // Opening curly bracket is known.
    ValueObject* object = new ValueObject;
    enum ParsingPart {Name, Colon, Value, Comma};
    ParsingPart parsing_part = Name;
    std::string key;
    for (; !data.eof(); ++data) {
        char c = data.get();
        if (WHITESPACE.count(c)) {
            continue;
        }
        switch (parsing_part) {
            case Name:
                // Allow for potential empty object.
                if (object->empty() && STRUCTURAL_CHARS.count(c)) {
                    if (STRUCTURAL_CHARS[c] == Structural::end_object) {
                        // End of object successfully reached.
                        return std::make_unique<Object>(ValueType::object, object);
                    }
                }  else if (c == STRING_QUOTES) {
                    // Deduce string key, ensuring not duplicate.
                    // Retain it to map it to the corresponding value to come.
                    key = *(std::string*)(parse_string(data)->value());
                    if (object->count(key)) {
                        // Duplicate key, typically disallowed in JSON.
                        throw;
                    }
                } else {
                    throw;
                }
                break;
            case Colon:
                if (!STRUCTURAL_CHARS.count(c) || STRUCTURAL_CHARS[c] != Structural::name_separator) {
                    // Not a colon as expected.
                    throw;
                }
                break;
            case Value:
                object->operator[](key) = parse_value(data);
                break;
            case Comma:
                if (!STRUCTURAL_CHARS.count(c)) {
                    throw;
                }
                switch (STRUCTURAL_CHARS[c]) {
                    case Structural::end_object:
                        // Object ends here instead of another comma.
                        return std::make_unique<Object>(ValueType::object, object);
                    case Structural::value_separator:
                        break;
                    default:
                        throw;
                }
                break;
        }
        // To the next parsing part, cycling back to Name after Comma.
        parsing_part = (ParsingPart)((parsing_part + 1) % 4);
    }
    // End of string reached without object closure.
    throw;
}


ValuePtr parse_number(_DataWrapper& data) {
    // Optional minus sign.
    bool negative = data.get() == MINUS_SIGN;
    if (negative) {
        ++data;
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
    for (; !data.eof(); ++data) {
        char c = data.get();
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
    --data;
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


// Fill UTF-8 byte with bit values (not the hard-coded constant bits).
// Mutates the byte (fills it in) and code point (discards LSB per iteration).
inline void fill_utf8_byte(unsigned char& byte, int& code_point, int count) {
    for (int _ = 0; _ < count; ++_) {
        byte = (byte << 1) + (code_point & 1);
        code_point >>= 1;
    }
}


ValuePtr parse_string(_DataWrapper& data) {
    ++data; // Opening double quote.
    std::string* result = new std::string;
    bool escape = false;
    bool unicode_escape = false;
    int unicode_code_point = 0;
    int unicode_escape_chars = 0;
    for (; !data.eof(); ++data) {
        char c = data.get();
        if (unicode_escape) {
            // Handles Unicode escape character (\uXXXX).
            if (!isxdigit(c)) {
                // Not valid hex character.
                throw;
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
                    result->push_back(unicode_code_point);
                } else if (unicode_code_point <= 0x07FF) {
                    // Two bytes: 110xxxxx 10xxxxxx
                    unsigned char byte1 = 0;
                    unsigned char byte2 = 0;
                    fill_utf8_byte(byte2, unicode_code_point, 6);
                    fill_utf8_byte(byte1, unicode_code_point, 5);
                    byte1 |= 0b11000000;
                    byte2 |= 0b10000000;
                    result->append({char(byte1), char(byte2)});
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
                    result->append({char(byte1), char(byte2), char(byte3)});
                }
                unicode_escape = false;
                unicode_code_point = 0;
                unicode_escape_chars = 0;
            }
        } else if (escape) {
            // Handle escape character without yet knowing what is to come.
            if (ESCAPE_CHARS.count(c)) {
                result->push_back(ESCAPE_CHARS[c]);
            } else if (c == UNICODE_ESCAPE) {
                unicode_escape = true;
                escape = false;
            } else {
                // Invalid escape character.
                throw;
            }
            escape = false; // Either escape character done, or transfer control to Unicode escape.
        } else {
            // Handle normal character.
            switch (c) {
                case STRING_QUOTES:
                    // The string has been closed.
                    return std::make_unique<Value>(ValueType::string, result);
                case BACKSLASH:
                    // Start of an escaped character.
                    escape = true;
                    break;
                default:
                    // Just a normal character - append.
                    result->push_back(c);
            }
        }
    }
    // End of JSON without closing double quote - erroneous.
    throw;
}


ValuePtr parse_literal_name(_DataWrapper& data) {
    std::string literal_name = "";
    for (; !data.eof(); ++data) {
        literal_name += data.get();
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


ValuePtr parse_json(_DataWrapper& data) {
    ValuePtr result = nullptr;
    for (; !data.eof(); ++data) {
        if (WHITESPACE.count(data.get())) {
            // Ignore insignificant whitespace.
            continue;
        }
        if (result != nullptr) {
            // Cannot have second part of the toplevel data... error
            throw;
        }
        result = parse_value(data);
    }
    if (result == nullptr) {
        // Nothing found but whitespace.
        throw;
    }
    return result;
}


ValuePtr parse_json(const std::string& string) {
    _StrWrapper wrapper(string);
    return parse_json(wrapper);
}


ValuePtr parse_json(std::istream& istream) {
    _IstreamWrapper wrapper(istream);
    return parse_json(wrapper);
}
