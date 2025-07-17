#pragma once
#include <tuple>
#include <stdexcept>
#include <functional>
#include <type_traits>
#include <string_view>

namespace func_invoke {

    class key_not_found_error : public std::runtime_error {
    public:
        explicit key_not_found_error(std::string_view key_name)
            : std::runtime_error("Key '" + std::string(key_name) + "' not found.") {}
    };

    class type_mismatch_error : public std::runtime_error {
    public:
        type_mismatch_error(std::string_view key_name, std::string_view type_name, std::string_view error_msg)
            : std::runtime_error(
                "Type mismatch for key '" + std::string(key_name) + "'. Expected: "
                + std::string(type_name) + ". JSON error: " + std::string(error_msg)
            ) {
        }
    };

    template<typename T>
    struct value_extractor_tag {};

    template <typename T, typename Container>
    T extract_value(value_extractor_tag<T>, const Container& c, std::string_view key);

    struct KeyArg {
        std::string_view key;
        constexpr KeyArg(const char* str) : key(str) {}
        constexpr explicit KeyArg(std::string_view _key) : key(_key) {}
    };

    namespace detail {
        template <typename T>
        struct FunctionTraits;
        template <typename R, typename... Args>
        struct FunctionTraits<R(Args...)> { using args_tuple = std::tuple<std::decay_t<Args>...>; };
        template <typename R, typename... Args>
        struct FunctionTraits<R(*)(Args...)> : FunctionTraits<R(Args...)> {};
        template <typename R, typename... Args>
        struct FunctionTraits<std::function<R(Args...)>> : FunctionTraits<R(Args...)> {};
        template <typename C, typename R, typename... Args>
        struct FunctionTraits<R(C::*)(Args...)> : FunctionTraits<R(Args...)> {};
        template <typename C, typename R, typename... Args>
        struct FunctionTraits<R(C::*)(Args...) const> : FunctionTraits<R(Args...)> {};
        template <typename T>
        struct FunctionTraits : FunctionTraits<decltype(&T::operator())> {};

        // extract a value of type T from container by key
        template <typename T, typename Container>
        T extract_value_impl(const Container& c, std::string_view key) {
            return extract_value(value_extractor_tag<T>{}, c, key);
        }

        // Select an argument: if it's KeyArg, extract expected type, else pass through
        template <typename Expected, typename Container, typename Arg>
        auto select_arg(const Container& c, Arg&& a) {
            if constexpr (std::is_same_v<std::decay_t<Arg>, KeyArg>) {
                return extract_value_impl<Expected>(c, a.key);
            }
            else {
                return std::forward<Arg>(a);
            }
        }

        // Рекурсивное построение кортежа аргументов из кортежей ключей и типов
        template <typename Container, typename TupleExpected, typename... Keys, size_t... Is>
        auto build_arg_tuple_impl(const Container& c, std::index_sequence<Is...>, TupleExpected const&, Keys&&... keys) {
            auto key_tuple = std::forward_as_tuple(std::forward<Keys>(keys)...);
            return std::make_tuple(select_arg<std::tuple_element_t<Is, TupleExpected>>(c, std::get<Is>(key_tuple))...);
        }

        template <typename Container, typename... Expected, typename... Keys>
        auto build_arg_tuple(const Container& c, const std::tuple<Expected...>& expected, Keys&&... keys) {
            static_assert(sizeof...(Expected) == sizeof...(Keys), "Mismatch between expected types count and keys count");
            return build_arg_tuple_impl<Container, std::tuple<Expected...>>
                (c, std::index_sequence_for<Expected...>{}, expected, std::forward<Keys>(keys)...);
        }

    } // namespace detail

    // --- invoke для свободной функции ---
    template <typename Fn, typename Container, typename... Keys, typename = std::enable_if_t<!std::is_member_pointer_v<std::decay_t<Fn>>>>
    void invoke(const Container& c, Fn&& func, Keys&&... keys) {
        using traits = detail::FunctionTraits<std::decay_t<Fn>>;
        using expected_tuple = typename traits::args_tuple;

        // Создаём кортежы: один с типами, другой — с ключами
        expected_tuple expected{};
        auto args = detail::build_arg_tuple(c, expected, std::forward<Keys>(keys)...);
        std::apply(std::forward<Fn>(func), std::move(args));
    }

    // --- invoke для метода ---
    template <typename Fn, typename Obj, typename Container, typename... Keys, typename = std::enable_if_t<std::is_member_pointer_v<std::decay_t<Fn>>>>
    void invoke(const Container& c, Fn method, Obj&& obj, Keys&&... keys) {
        using traits = detail::FunctionTraits<std::decay_t<Fn>>;
        using expected_tuple = typename traits::args_tuple;

        expected_tuple expected{};
        auto args_tuple = detail::build_arg_tuple(c, expected, std::forward<Keys>(keys)...);
        std::apply([&](auto&&... unpacked) { (std::forward<Obj>(obj).*method)(std::forward<decltype(unpacked)>(unpacked)...); }, std::move(args_tuple));
    }

} // namespace func_invoke
