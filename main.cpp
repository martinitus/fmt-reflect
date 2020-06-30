#include <iostream>
#include <sstream>
#include <stdexcept>
#include <variant>

#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace std::string_literals;

struct Inner {
    int x;
    double y;
    std::string z;
};

struct Outer {
    std::string a;
    std::string b;
    Inner inner;
};

template <typename T>
struct Simple {};

template <typename T>
struct Extended {};

/**
 * Structure that holds reflection information about type T.
 * @tparam T
 */
template <typename T>
struct reflection {
    static constexpr bool available = false;
};

template <typename T>
struct NamedField {
    const std::string name;
    const T& value;
};

template <>
struct reflection<Outer> {
    static constexpr bool available = true;
    static auto name() { return "Outer"; }
    static auto fields(const Outer& outer) {
        return std::tuple{
            NamedField<std::string>{"a", outer.a},
            NamedField<std::string>{"b", outer.b},
            NamedField<Inner>{"inner", outer.inner},
        };
    }
    static auto values(const Outer& outer) { return std::tuple{outer.a, outer.b, outer.inner}; }
};

template <>
struct reflection<Inner> {
    static constexpr bool available = true;
    static auto name() { return "Inner"; }
    static auto fields(const Inner& inner) {
        return std::tuple{
            NamedField<int>{"x", inner.x},
            NamedField<double>{"y", inner.y},
            NamedField<std::string>{"z", inner.z},
        };
    }

    static auto values(const Inner& inner) { return std::tuple{inner.x, inner.y, inner.z}; }
};

template <typename T>
struct fmt::formatter<NamedField<T>> {
    template <typename FormatContext>
    auto format(NamedField<T> const& t, FormatContext& ctx) {
        // fixme this is hacky, as we reference the extended format specifier from within the extended formating
        //       implementation. Would probably be better to visit all name field elements directly in the Extended<T>
        //       formatter specialization
        if constexpr (reflection<T>::available) {
            return fmt::format_to(ctx.out(), ".{}={:e}", t.name, t.value);
        } else {
            return fmt::format_to(ctx.out(), ".{}={}", t.name, t.value);
        }
    }
};

/**
 * Formats objects as concatenated list of their values.
 *
 * E.g. fmt::format("{:s}", Outer{1,2,Inner{3,4,5}}) will output 1|2|3|4|5
 *
 *
 * @tparam T The object to format.
 * @tparam C The character type used.
 */
template <typename T, typename C>
struct fmt::formatter<Simple<T>, C, std::enable_if_t<reflection<T>::available, void>> {
    template <typename FormatContext>
    auto format(T const& t, FormatContext& ctx) {
        using base = formatter<decltype(fmt::join(reflection<T>::values(t), "|"))>;
        return base{}.format(fmt::join(reflection<T>::values(t), "|"), ctx);
    }
};

/**
 * Formats objects as they would be written using designated initializers.
 *
 * E.g. {:e} will output Outer{.a=1, b=2, .inner=Inner{.x=3, .y=4, .z=5}}
 *
 * @tparam T The object to format.
 * @tparam C The character type used.
 */
template <typename T, typename C>
struct fmt::formatter<Extended<T>, C, std::enable_if_t<reflection<T>::available, void>> {
    template <typename FormatContext>
    auto format(T const& t, FormatContext& ctx) {
        std::string name = reflection<T>::name();

        std::copy(name.begin(), name.end(), ctx.out());
        *ctx.out()++ = '{';
        using base = formatter<decltype(fmt::join(reflection<T>::fields(t), ", "))>;
        base{}.format(fmt::join(reflection<T>::fields(t), ", "), ctx);
        *ctx.out()++ = '}';
        return ctx.out();
    }
};

template <typename T, typename Char>
struct fmt::formatter<T, Char, std::enable_if_t<reflection<T>::available, void>> {
    using simple_fmt = fmt::formatter<Simple<T>>;
    using extended_fmt = fmt::formatter<Extended<T>>;
    using underlying_formatter_type = std::variant<simple_fmt, extended_fmt>;

    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        // Parse the presentation format and store it in the formatter:
        auto it = ctx.begin(), end = ctx.end();
        if (*it == 's') {
            underlying_formatter = simple_fmt{};
            it++;
        } else if (*it == 'e') {
            underlying_formatter = extended_fmt{};
            it++;
        } else {
            throw format_error("invalid format");
        }

        // Check if reached the end of the range:
        if (it != end && *it != '}') throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }

    template <typename FormatContext>
    auto format(T const& t, FormatContext& ctx) {
        return std::visit([&](auto& fmt) { return fmt.format(t, ctx); }, underlying_formatter);
    }

   private:
    underlying_formatter_type underlying_formatter;
};

int main() {
    auto outer = Outer{.a = "a", .b = "b", .inner = {.x = 1, .y = 3.14, .z = "z"}};
    try {
        std::stringstream extended;
        extended << "Outer{.a=" << outer.a << ", .b=" << outer.b << ", .inner=Inner{.x=" << outer.inner.x
                 << ", .y=" << outer.inner.y << ", .z=" << outer.inner.z << "}}";

        std::stringstream simple;
        simple << outer.a << "|" << outer.b << "|" << outer.inner.x << "|" << outer.inner.y << "|" << outer.inner.z;

        std::cout << "Manual extended: " << extended.str() << std::endl;
        std::cout << "libfmt extended: " << fmt::format("{:e}", outer) << std::endl;
        std::cout << "Manual simple: " << simple.str() << std::endl;
        std::cout << "libfmt simple: " << fmt::format("{:s{;}}", outer) << std::endl;

        bool extended_fail = fmt::format("{:e}", outer) != extended.str();
        bool simple_fail = fmt::format("{:s}", outer) != simple.str();
        if (extended_fail) {
            std::cerr << "extended format does not match expected output" << std::endl;
        }
        if (simple_fail) {
            std::cerr << "simple format does not match expected output" << std::endl;
        }
        return simple_fail || extended_fail ? -2 : 0;
    } catch (std::exception& e) {
        std::cerr << "Error:" << e.what() << std::endl;
        return -1;
    }
}