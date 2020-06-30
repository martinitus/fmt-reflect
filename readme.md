# Formatting user objects with `libfmt`

This repo shows an example how to use [libfmt](https://github.com/fmtlib/fmt) and _manually configured static reflection_ to format user defined 
structures.

Essentially it provides template specializations for `fmt::formatter` that allow formatting of structs for which 
reflection information was setup via the `reflection` template.

Thanks to [this blog](https://wgml.pl/blog/formatting-user-defined-types-fmt.html) which helped a lot :-) 

Example: 

```c++

struct Inner {...};
struct Outer {...};

template<>
struct reflection<Outer> {
    /*definition of class name and field names has to provided manually...*/
};

template<>
struct reflection<Inner> {
    /*definition of class name and field names has to provided manually...*/
};

auto outer = Outer{.a=1,.b="hello",.inner={.x=3.12,.y=" ",.z=2}};

std::string simple = fmt::format("{:s}", outer); // :s means format as simple
assert(simple == "a|hello|3.12| |2");

std::string  extended = fmt::format("{:e}",outer); // :e means format as extended
assert(extended == "Outer{.a=1, .b=hello, .inner=Inner{.x=3.12, .y= , .z=2}}");
```


