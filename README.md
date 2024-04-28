# JSON Parser

**JSON** is a **lightweight**, simple way of storing and transferring basic structured data, focusing on readability and simplicity. In fact, it is widely used to transmit data over a **network** and can be used for **configuration files**. Thus, JSON parsers/generators have been implemented in countless programming languages and other programs, so this project is no new feat (even more restricted in the fact it will only parse JSON, not generate it).

Nonetheless, C++ is a language that does not support JSON natively, but there indeed exist robust third party libraries to handle JSON. Still, this project is for educational purposes only, to gain a deeper understanding of how JSON works, and for some basic, introductory parsing practice.

JSON is very dynamic whereas C++ is relatively rigid. Nonetheless, powerful usage of the **[std::variant](https://en.cppreference.com/w/cpp/utility/variant)** class allows for a safe, elegant way of representing JSON data in C++ without resorting to nasty alternatives such as void pointers. This is possible since JSON consists of a finite, manageable number of data types.

Features of this mini C++ JSON parser:
- Follows the **standard**: https://datatracker.ietf.org/doc/html/rfc8259
- Parses JSON from a `std::string` object.
- Parses JSON from a `std::istream` object (any valid standard C++ input stream, including file/string streams).
- Correctly parses and stores JSON objects, arrays, numbers, strings and literals.
- Provides a solid **error handling** scheme, including information on error positions where possible.
- Can easily check two JSON objects for **equality**, a built-in feature of `std::variant`.
- Decent performance with an elegant interface.

## Usage
The code has proven to work on **Windows 10, g++ v12.1.0, C++17**. However, there is no known use of implementation-defined behaviour nor OS-specific behaviour. It is expected the code should compile on **any reasonable OS and conforming compiler, with C++17 minimum**.

From the `src` folder, simply include the `json.h` header in your project and compile with the `json.cpp` file bundled, and you will be set. The parser does speed up with optimisations enabled during compilation.

To explore how to use the code, and how the code handles special JSON cases such as duplicate object keys, refer to [USAGE.md](USAGE.md)

## Disclaimer
Whilst performance is reasonable and the parser has been tested robustly, **bette JSON parsers exist** for C++. Use them if performance is important or robustness is vital, since this mini project may still contain hidden, rare bugs not caught by the non-exhaustive test cases.

But this parser can still be **quick and simple** to integrate into a project that handles **small JSON data**, particularly one that only needs to parse JSON and not generate it. For example, parsing a 1KB JSON file will probably take less than 100μs but this would depend on computer speed and type of data.

The code can be viewed and modified as you wish - there are no restrictions regarding the usage of the code. But there will be no liability due to any damages caused by the code.