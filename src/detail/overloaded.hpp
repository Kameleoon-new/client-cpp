#pragma once

namespace kameleoon::detail
{
    /// Merges several callables into one overload set, for `std::visit`:
    ///
    ///     std::visit(Overloaded{[](const A &) {...}, [](const B &) {...}}, variant);
    template <typename... Ts>
    struct Overloaded : Ts...
    {
        using Ts::operator()...;
    };

    template <typename... Ts>
    Overloaded(Ts...) -> Overloaded<Ts...>;

} // namespace kameleoon::detail
