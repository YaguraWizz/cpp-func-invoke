#pragma once
#include <tuple>
#include <array>        
#include <functional>
#include <type_traits>
#include <string_view>  

namespace func_invoke {

    template<typename T>
    struct value_extractor_tag {};

    template <typename T, typename Container>
    T extract_value(value_extractor_tag<T>, const Container& c, std::string_view key);

    namespace detail {

        template <typename T>
        struct FunctionTraits; 

        template <typename R, typename... Args>
        struct FunctionTraits<R(Args...)> {
            using args_tuple = std::tuple<std::decay_t<Args>...>;
        };

        template <typename R, typename... Args>
        struct FunctionTraits<R(*)(Args...)> : FunctionTraits<R(Args...)> {};

        template <typename R, typename... Args>
        struct FunctionTraits<std::function<R(Args...)>> : FunctionTraits<R(Args...)> {};

        template <typename C, typename R, typename... Args>
        struct FunctionTraits<R(C::*)(Args...)> {
            using args_tuple = std::tuple<std::decay_t<Args>...>;
            using class_type = C;
        };

        template <typename C, typename R, typename... Args>
        struct FunctionTraits<R(C::*)(Args...) const> {
            using args_tuple = std::tuple<std::decay_t<Args>...>;
            using class_type = C;
        };

        template <typename T>
        struct FunctionTraits : FunctionTraits<decltype(&T::operator())> {};

        template <typename T, typename Container>
        T extract_value_impl(const Container& c, std::string_view key) {
            return extract_value(value_extractor_tag<T>{}, c, key);
        }

        template <typename Tuple, std::size_t... Is, typename Container, std::size_t N>
        Tuple build_arg_tuple_impl(const Container& c, std::index_sequence<Is...>, const std::string_view(&keys)[N]) {
            return std::make_tuple(extract_value_impl<std::tuple_element_t<Is, Tuple>>(c, keys[Is])...);
        }

        template <typename Tuple, typename Container, std::size_t N>
        Tuple build_arg_tuple(const Container& c, const std::string_view(&keys)[N]) {
            return build_arg_tuple_impl<Tuple>(c, std::make_index_sequence<std::tuple_size_v<Tuple>>{}, keys);
        } 
    }
 
    // --- invoke для обычных функций ---
    template <typename Fn, typename Container, std::size_t N>
    void invoke(const Container& c, Fn&& func, const std::string_view(&keys)[N]) {
        using args_tuple = typename detail::FunctionTraits<std::decay_t<Fn>>::args_tuple;
        static_assert(std::tuple_size_v<args_tuple> == N,
            "Number of keys provided does not match the number of function arguments.");

        auto args = detail::build_arg_tuple<args_tuple>(c, keys);
        std::apply(std::forward<Fn>(func), args);
    }

    // --- invoke для member function ---
    template <typename Fn, typename Obj, typename Container, std::size_t N>
    void invoke(const Container& c, Fn method, Obj&& obj, const std::string_view(&keys)[N]) {
        using traits = detail::FunctionTraits<Fn>;
        using args_tuple = typename traits::args_tuple;
        static_assert(std::tuple_size_v<args_tuple> == N,
            "Number of keys provided does not match the number of member function arguments.");

        auto args = detail::build_arg_tuple<args_tuple>(c, keys);

        std::apply([&](auto&&... unpacked) {
                (obj.*method)(std::forward<decltype(unpacked)>(unpacked)...);
            }, args);
    }

} // namespace inv