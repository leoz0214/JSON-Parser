// Live testing of the JSON parser to ensure it is functioning correctly.
#include <functional>
#include <iostream>
#include <string>
#include <cassert>
#include <variant>
#include "../src/json.h"


void string_test(const std::string& string, std::function<void(const Value&)> function) {
    function(parse_json(string));
}


int main() {
    string_test("true", [](const Value& value) {
        assert(std::get<Boolean>(value) == true);
    });
    string_test("[]", [](const Value& value) {
        assert(std::get<Array>(value).empty());
    });
    string_test(R"("This is a Unicode string!\u00e9\u00e9\u00e9\u1234")", [](const Value& value) {
        assert(std::get<String>(value) ==  "This is a Unicode string!éééሴ");
    });
    string_test("[null, 1.25, \"52\", false]", [](const Value& value) {
        const Array& array = std::get<Array>(value);
        assert(array.size() == 4);
        assert(std::holds_alternative<Null>(array[0]));
        assert(std::get<Number>(array[1]) == 1.25);
        assert(std::get<String>(array[2]) == "52");
        assert(!std::get<Boolean>(array[3]));
    });
}