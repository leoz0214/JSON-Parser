// Testing of the JSON parser to ensure it is functioning correctly.
#include <iostream>
#include <functional>
#include <fstream>
#include <string>
#include <cassert>
#include <variant>
#include <filesystem>
#include <vector>
#include <utility>
#include "../src/json.h"


using namespace json;


std::string get_test_files_folder() {
    if (std::filesystem::directory_entry("testing").exists()) {
        return "testing/files";
    }
    return "files";
}
const std::string FILES_FOLDER = get_test_files_folder();


typedef std::function<void(const json::Value&)> TestFunction;
void string_test(const std::string& string, TestFunction function) {
    std::cout << string << '\n';
    function(parse(string));
}


void file_test(const std::string& file_name, TestFunction function) {
    std::ifstream file(FILES_FOLDER + "/" + file_name);
    assert(file.is_open());
    function(parse(file));
}


int main() {
    string_test("    true    ", [](const json::Value& value) {
        assert(std::get<Boolean>(value) == true);
    });
    string_test("[]", [](const Value& value) {
        assert(std::get<Array>(value).empty());
    });
    string_test(R"("This is a Unicode string!\u00e9\u00e9\u00e9\u1234")", [](const Value& value) {
        assert(std::get<String>(value) ==  "This is a Unicode string!éééሴ");
    });
    string_test("[null, 1.25,\"52\", false]", [](const Value& value) {
        const Array& array = std::get<Array>(value);
        assert(array.size() == 4);
        assert(std::holds_alternative<Null>(array[0]));
        assert(std::get<Number>(array[1]) == 1.25);
        assert(std::get<String>(array[2]) == "52");
        assert(!std::get<Boolean>(array[3]));
    });
    string_test(R"( "\n\t\n\\\/\b")", [](const Value& value) {
        assert(std::get<String>(value) == "\n\t\n\\/\b");
    });
    string_test("{}", [](const Value& value) {
        assert(std::get<Object>(value).empty());
    });
    string_test("{\"123\": 456, \"Hello\": \"World!\"}", [](const Value& value) {
        const Object& obj = std::get<Object>(value);
        assert(std::get<Number>(obj.at("123")) == 456);
        assert(std::get<String>(obj.at("Hello")) == "World!");
        assert((value == parse("{\"Hello\": \"World!\",\"123\":456}")));
    });
    // In this implementation, allow duplicate keys.
    string_test("{\"a\": 25, \"b\": 24, \"a\": 3.14}", [](const Value& value) {
        assert(std::get<Number>(std::get<Object>(value).at("a")) == 3.14);
    });

    file_test("basic.json", [](const Value& value) {
        auto arr = std::get<Array>(value);
        assert(std::get<Number>(arr[1]) == 3e5);
        assert(std::holds_alternative<Null>(arr[2]));
        assert(std::get<String>(arr[3]) == "Hello, JSON!");
    });
    file_test("extremely_deep_array.json", [](auto) {});
    file_test("example.json", [](const Value& value) {
        const Object& obj = std::get<Object>(value);
        String url = std::get<String>(
            std::get<Object>(
                std::get<Object>(obj.at("Image")
            ).at("Thumbnail")
        ).at("Url"));
        assert(url == "http://www.example.com/image/481989943");
        Array ids = std::get<Array>(std::get<Object>(obj.at("Image")).at("IDs"));
        assert(ids.size() == 4);
        assert(!obj.count("image"));
    });
    file_test("config.json", [](const Value& value) {
        struct Config {
            std::string sender;
            std::string password;
            std::vector<std::string> recipients;
            std::vector<std::pair<std::string, std::string>> day_times;

            Config(const Value& value) {
                const Object& obj = std::get<Object>(value);
                this->sender = std::get<std::string>(obj.at("sender"));
                this->password = std::get<std::string>(obj.at("password"));
                for (const Value& recipient : std::get<Array>(obj.at("recipients"))) {
                    this->recipients.push_back(std::get<std::string>(recipient));
                }
                for (const Value& day_time : std::get<Array>(obj.at("day_times"))) {
                    const Array& day_time_arr = std::get<Array>(day_time);
                    this->day_times.push_back(
                        {std::get<std::string>(day_time_arr[0]),
                        std::get<std::string>(day_time_arr[1])}
                    );
                }
            }
        };
        Config config(value);
        assert(config.sender == "example@gmail.com");
        assert(config.password == "***");
        assert(config.recipients[1] == "b@yahoo.com" && config.recipients.size() == 3);
        assert(config.day_times[0].second == "2200");
        assert(value == value);
    });
    file_test("complex_config.json", [](const Value& value) {
        const Object& o = std::get<Object>(value);
        assert((int)std::get<Number>(o.at("client_id")) == 999999999);
        assert((int)std::get<Number>(std::get<Object>(o.at("hr_zones")).at("4")) == 160);
        assert(std::get<Object>(std::get<Object>(o.at("markers")).at("moving_time")).empty());
        auto markers = std::get<Object>(o.at("markers"));
        auto start_time_markers = std::get<Object>(markers.at("start_time"));
        for (const auto& [key, intervals] : start_time_markers) {
            for (const auto& interval : std::get<Array>(intervals)) {
                for (const auto& val : std::get<Array>(interval)) {
                    assert(std::holds_alternative<String>(val));
                }
            } 
        }
        auto route_templates = std::get<Array>(o.at("route_templates"));
        auto route_template = std::get<Object>(route_templates[0]);
        assert(std::holds_alternative<Null>(route_template.at("priority")));
        assert(value == value);
    });
}