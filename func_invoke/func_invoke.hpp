#pragma once
#include <tuple>
#include <stdexcept>
#include <functional>
#include <type_traits>
#include <string_view>

namespace func_invoke {
    namespace traits {
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
    }
    namespace exeption_error {
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
    }
    using namespace exeption_error;

    template<typename T>
    struct value_extractor_tag {};

    template <typename T, typename Container>
    T extract_value(value_extractor_tag<T>, const Container& c, std::string_view key);

    struct KeyArg {
        std::string_view key;
        constexpr KeyArg(const char* str) : key(str) {}
        constexpr explicit KeyArg(std::string_view _key) : key(_key) {}
    };

#if _HAS_CXX20
    template <std::size_t N>
    struct fixed_string {
        char value[N];

        constexpr fixed_string(const char(&str)[N]) {
            for (std::size_t i = 0; i < N; ++i)
                value[i] = str[i];
        }
        constexpr operator std::string_view() const {
            return std::string_view(value, N - 1); 
        }
        constexpr bool operator==(const fixed_string& other) const {
            for (std::size_t i = 0; i < N; ++i)
                if (value[i] != other.value[i])
                    return false;
            return true;
        }
        constexpr bool operator!=(const fixed_string& other) const {
            return !(*this == other);
        }
    };

    template <fixed_string Key, typename T> 
    struct Value {
        using value_type = T;

        Value() = default;
        Value(const T& value) : _value(value) {}
        Value(T&& value) : _value(std::move(value)) {}

        const T& data() const noexcept { return _value; }
        T& data() { return _value; }

        static constexpr std::string_view key() { return Key; } 
    private:
        T _value;
    };

    namespace traits {
        template <typename T>
        struct is_value_type : std::false_type {};

        // Частичная специализация для Value<>
        template <fixed_string Key, typename U>
        struct is_value_type<Value<Key, U>> : std::true_type {};
    }
#endif



    namespace detail {
        using namespace traits;

        template <typename... Ts>
        struct count_non_value_types;

        template <>
        struct count_non_value_types<> {
            static constexpr size_t value = 0;
        };

        template <typename T, typename... Ts>
        struct count_non_value_types<T, Ts...> {
            static constexpr size_t value = (!is_value_type<T>::value ? 1 : 0) + count_non_value_types<Ts...>::value;
        };


        // extract a value of type T from container by key
        template <typename T, typename Container>
        T extract_value_impl(const Container& c, std::string_view key) {
            return extract_value(value_extractor_tag<T>{}, c, key);
        }

        // Select an argument: if it's KeyArg, extract expected type, else pass through
        // Выбор аргумента в зависимости от его типа
        template <typename Expected, typename Container, typename Arg>
        auto select_arg(const Container& c, Arg&& a) {
#if _HAS_CXX20
            if constexpr (is_value_type<Expected>::value) {
                using value_type = typename Expected::value_type;
                return extract_value_impl<value_type>(c, Expected::key());
            }
            else
#endif
                if constexpr (std::is_same_v<std::decay_t<Arg>, KeyArg>) {
                    return extract_value_impl<Expected>(c, a.key);
                }
                else {
                    return std::forward<Arg>(a);
                }
        }

        template <size_t I, size_t J, typename Container, typename TupleExpected, typename TupleArgs, typename... Accumulated>
        auto build_arg_tuple_recursive(const Container& c, const TupleExpected& expected, TupleArgs&& args_tuple, Accumulated&&... acc) {           
            if constexpr (I == std::tuple_size_v<TupleExpected>) {
                return std::make_tuple(std::forward<Accumulated>(acc)...);
            }
            else {
                using ExpectedType = std::tuple_element_t<I, TupleExpected>;

                if constexpr (is_value_type<ExpectedType>::value) {
                    // We only use ExpectedType to extract the value
                    auto val = select_arg<ExpectedType>(c, 0); 
                    return build_arg_tuple_recursive<I + 1, J>(c, expected, std::forward<TupleArgs>(args_tuple),
                        std::forward<Accumulated>(acc)..., std::move(val));
                }
                else {
                    // We get the argument from args_tuple at the index J
                    auto&& arg = std::get<J>(std::forward<TupleArgs>(args_tuple));
                    auto val = select_arg<ExpectedType>(c, std::forward<decltype(arg)>(arg));
                    return build_arg_tuple_recursive<I + 1, J + 1>(c, expected, std::forward<TupleArgs>(args_tuple),
                        std::forward<Accumulated>(acc)..., std::move(val));
                }
            }
        }


        template <typename Container, typename... Expected, typename... Args>
        auto build_arg_tuple(const Container& c, const std::tuple<Expected...>& expected, Args&&... args) {
#if _HAS_CXX20
            constexpr size_t needed_args = count_non_value_types<Expected...>::value;
            static_assert(sizeof...(Args) == needed_args, "Mismatch between provided arguments and expected non-value_type arguments");
#else
            static_assert(sizeof...(Args) == sizeof...(Expected), "Mismatch between expected types and argument count");
#endif

            auto args_tuple = std::forward_as_tuple(std::forward<Args>(args)...);
            return build_arg_tuple_recursive<0, 0>(c, expected, std::move(args_tuple));
        }


       


    } // namespace detail




    // --- invoke для свободной функции ---
    template <typename Fn, typename Container, typename... Args, typename = std::enable_if_t<!std::is_member_pointer_v<std::decay_t<Fn>>>>
    void invoke(const Container& c, Fn&& func, Args&&... args) {
        using traits = detail::FunctionTraits<std::decay_t<Fn>>;
        using expected_tuple = typename traits::args_tuple;

        expected_tuple expected{};
        auto args_tuple = detail::build_arg_tuple(c, expected, std::forward<Args>(args)...);
        std::apply(std::forward<Fn>(func), std::move(args_tuple));
    }

    // --- invoke для метода ---
    template <typename Fn, typename Obj, typename Container, typename... Args, typename = std::enable_if_t<std::is_member_pointer_v<std::decay_t<Fn>>>>
    void invoke(const Container& c, Fn method, Obj&& obj, Args&&... args) {
        using traits = detail::FunctionTraits<std::decay_t<Fn>>;
        using expected_tuple = typename traits::args_tuple;

        expected_tuple expected{};
        auto args_tuple = detail::build_arg_tuple(c, expected, std::forward<Args>(args)...);
        std::apply([&](auto&&... unpacked) { (std::forward<Obj>(obj).*method)(std::forward<decltype(unpacked)>(unpacked)...); }, std::move(args_tuple));
    }

} // namespace func_invoke