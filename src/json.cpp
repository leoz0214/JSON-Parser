// JSON parser implementation as per https://datatracker.ietf.org/doc/html/rfc8259
#include "json.h"


Value::Value(ValueType type, void* value) {
    this->_type = type;
    this->_value = value;
}

ValueType Value::type() const {
    return this->_type;
}

void* const Value::value() const {
    return this->_value;
}


Value parse_array(const std::string& string, int& i) {
    i++;
}


Value parse_object(const std::string& string, int& i) {
    i++;
}


Value parse_number(const std::string& string, int& i) {
    
}


Value parse_string(const std::string& string, int& i) {
    i++;
}


Value parse_literal_name(const std::string& string, int& i) {
    std::string literal_name = "";
    for (; i < string.size(); ++i) {
        literal_name += string[i];
        if (literal_name.size() > 5 ) {
            throw;
        }
        if (LITERAL_NAME_MAP.count(literal_name)) {
            i++;
            return Value(ValueType::literal_name, LITERAL_NAME_MAP[literal_name]);
        }
    }
    throw;
}


Value parse_json(const std::string& string) {
    Value result;
    bool empty = true;
    for (int i = 0; i < string.size(); ++i) {
        char c = string[i];
        if (WHITESPACE.count(c)) {
            // Ignore insignificant whitespace.
            continue;
        }
        if (!empty) {
            // Cannot have second part of the toplevel data... error
            throw;
        }
        empty = false;
        if (STRUCTURAL_CHARS.count(c)) {
            Structural structural_char = STRUCTURAL_CHARS[c];
            // In this context, either array/object opening is expected.
            // Anything else is erroneous.
            switch (structural_char) {
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
    if (empty) {
        throw;
    }
    return result;
}