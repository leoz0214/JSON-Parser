// Tries to break the parser.
// If the parser successfully raises suitable error messages, all is good.
#include <iostream>
#include <string>
#include <functional>
#include <stdexcept>
#include <cassert>
#include "../src/json.h"


typedef std::function<void(JsonParseError)> ErrorHandler;
void string_test(const std::string& string, ErrorHandler error_callback = nullptr) {
    try {
        parse_json(string);
        // This should not run or else the parser accepts erroneous data.
        throw std::runtime_error("Invalid string accepted by parser.");
    } catch (const JsonParseError& e) {
        if (error_callback != nullptr) {
            error_callback(e);
        }
    }
}


void check_correct_position(JsonParseError e, int pos) {
    std::cout << "Testing " << e.what() << '\n';
    assert(std::string(e.what()).find(" " + std::to_string(pos) + ":") != std::string::npos);
}


int main() {
    string_test("       ", [](JsonParseError e) {
        assert(e.what() == INVALID_JSON_DATA);
    });
    string_test("[1, 2,3][0]",  [](JsonParseError e) {
        assert(e.what() == INVALID_JSON_DATA);
    });
    std::string basic_invalid_jsons[] {
        "", "#", " ", "   \n\n\n \t", "[1,3.3,[]", " True ", "()", "00.00",
        "\"Hello", "\"Illegal es\\cape\"", "\"Bad Unic\\U0000",
        "\"\\udefg\"", "-.1", "3.", "+1000", "{\"\":null" 
    };
    for (const std::string& string : basic_invalid_jsons) {
        string_test(string);
    }
    string_test("[troeeeeeeeee]", [](JsonParseError e) {
        check_correct_position(e, 1);
    });
    string_test(" [ \"Abcdef\\N\"]", [](JsonParseError e) {
        check_correct_position(e, 11);
    });
    string_test("{\"Test\\uffZf\"}", [](JsonParseError e) {
        check_correct_position(e, 10);
    });
    string_test("\"123", [](JsonParseError e) {
        check_correct_position(e, 4);
    });
    string_test("00000000000000000000", [](JsonParseError e) {
        check_correct_position(e, 0);
    });
    string_test("[\"1\",-3.1416 E-34]", [](JsonParseError e) {
        check_correct_position(e, 13);
    });
    string_test("{{}: {{{{{}}}}}}", [](JsonParseError e) {
        check_correct_position(e, 1);
    });
    string_test("{\"abc\": true, \"abc\": false}", [](JsonParseError e) {
        check_correct_position(e, 14);
    });
    string_test(" {\" \"[1,2,3]} ", [](JsonParseError e) {
        check_correct_position(e, 5);
    });
    string_test("{\"\": [];}", [](JsonParseError e) {
        check_correct_position(e, 7);
    });
    string_test("[1,2,3,4.0;5,6,7]", [](JsonParseError e) {
        check_correct_position(e, 10);
    });
    string_test(" [5, ]", [](JsonParseError e) {
        check_correct_position(e, 5);
    });
    string_test("[[[[[[<)]]]]]]", [](JsonParseError e) {
        check_correct_position(e, 6);
    });
}