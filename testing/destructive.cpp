// Tries to break the parser.
// If the parser successfully raises suitable error messages, all is good.
#include <iostream>
#include <string>
#include <functional>
#include <stdexcept>
#include <cassert>
#include <fstream>
#include <filesystem>
#include "../src/json.h"


using namespace json;


std::string get_destructive_test_files_folder() {
    if (std::filesystem::directory_entry("testing").exists()) {
        return "testing/destructive";
    }
    return "destructive";
}
const std::string FILES_FOLDER = get_destructive_test_files_folder();


typedef std::function<void(JsonParseError)> ErrorHandler;
void string_test(const std::string& string, ErrorHandler error_callback = nullptr) {
    try {
        parse(string);
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


void file_test(const std::string& file_name, ErrorHandler error_callback = nullptr) {
    std::ifstream file(FILES_FOLDER + "/" + file_name);
    assert(file.is_open());
    try {
        parse(file);
        throw std::runtime_error("Invalid stream accepted by parser.");
    } catch (const JsonParseError& e) {
        if (error_callback != nullptr) {
            error_callback(e);
        }
    }
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
    string_test("1.05e+-2");

    file_test("empty", [](JsonParseError e) {
        assert(e.what() == INVALID_JSON_DATA);
    });
    file_test("invalid_literal");
    file_test("invalid_number", [](JsonParseError e) {
        check_correct_position(e, 4);
    });
    file_test("invalid_string", [](JsonParseError e) {
        check_correct_position(e, 114);
    });
    file_test("precision", [](JsonParseError e) {
        check_correct_position(e, 165);
        assert(std::string(e.what()).find("comma") != std::string::npos);
    });
}