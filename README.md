# JSON Parser

JSON is a lightweight, simple way of storing and transferring basic structured data, focusing on readability and simplicity. In fact, it is widely used to transmit data over a network and can be used for configuration files. Thus, JSON parsers/generators have been implemented in countless programming languages and other programs, so this project is no new feat (even more restricted in the fact it only will parse JSON, not generate it).

Nonetheless, C++ is a language that does not support JSON natively, but there indeed exist robust third party libraries to handle JSON. Still, the project is for educational purposes only, to gain a deeper understanding of how JSON works, and for some basic parsing practice.

Target (planned) features for this mini C++ JSON parser:
- Follow the standard: https://datatracker.ietf.org/doc/html/rfc8259
- Parse JSON from stream (file/string stream).
- Parse JSON from a string object.
- Consider JSON objects, arrays, integers, strings and literals.
- Handle numbers sensibly (as close to the standard as possible, ideally meeting it).
- Handle strings sensibly (again as conforming as possible).
- Solid error handling scheme.

Expected roadblocks:
- It will be a challenge to provide a suitable representation in C++ due to the static nature of the language. Nonetheless, polymorphic magic and the restricted set of possible types makes the task more feasible.
- Pain involving handling Unicode (UTF-8) as required by the JSON standard. String escaping madness.

Overall, a good, gentle start in understanding the parsing process.
2-3 weeks should suffice as JSON is not too complicated at all to parse, but strings in particular are non-trivial.
