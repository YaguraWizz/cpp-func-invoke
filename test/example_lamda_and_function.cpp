#include <iostream>
#include <string>
#include <functional>
#include <nlohmann/json.hpp> 
#include "func_invoke.hpp" 

namespace js = nlohmann;

// Function to extract values from nlohmann::json
// Must be defined in func_invoke namespace or globally
namespace func_invoke {
    template<typename T>
    static T extract_value(value_extractor_tag<T>, const nlohmann::json& jv, std::string_view key) {
        if (!jv.is_object() || !jv.contains(key.data())) {
            throw key_not_found_error(key);
        }
        const nlohmann::json& value_from_json = jv.at(key.data());
        try {
            return value_from_json.get<T>();
        }
        catch (const nlohmann::json::exception& ex) {
            throw type_mismatch_error(key, typeid(T).name(), ex.what());
        }
    }
} // namespace func_invoke

// Example free function
static void greet_user(int id, const std::string& name, double score) {
    std::cout << "Hello, " << name << "! (ID: " << id << ", Score: " << score << ")\n";
}

int main() {
    using namespace func_invoke;

    js::json data_json = {
        {"user_id", 123},
        {"user_name", "Alice"},
        {"score_val", 99.5}
    };

    // Example 1: Calling a free function with all arguments from JSON
    std::cout << "--- Calling greet_user (all arguments from JSON) ---\n";
    try {
        invoke(data_json, greet_user, KeyArg("user_id"), KeyArg("user_name"), KeyArg("score_val"));
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Example 2: Calling a free function with mixed arguments
    js::json partial_json = { {"user_name", "Bob"} };
    std::cout << "--- Calling greet_user (ID and Score direct, Name from JSON) ---\n";
    try {
        invoke(partial_json, greet_user, 456, KeyArg("user_name"), 75.0);
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Example 3: Calling a lambda expression
    js::json lambda_json = { {"value_a", 10}, {"is_active", true} };
    std::cout << "--- Calling a lambda expression ---\n";
    try {
        auto process_lambda = [](int a, double b, bool c) {
            std::cout << "Lambda called: a=" << a << ", b=" << b << ", c=" << std::boolalpha << c << "\n";
            };
        invoke(lambda_json, process_lambda, KeyArg("value_a"), 20.5, KeyArg("is_active"));
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Example 4: Error handling - missing key
    js::json missing_key_json = { {"user_id", 789}, {"score_val", 50.0} }; // "user_name" is missing
    std::cout << "--- Error: Missing key 'user_name' ---\n";
    try {
        invoke(missing_key_json, greet_user, KeyArg("user_id"), KeyArg("user_name"), KeyArg("score_val"));
    }
    catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Example 5: Error handling - type mismatch
    js::json type_mismatch_json = { {"user_id", 111}, {"user_name", "Charlie"}, {"score_val", "fifty"} }; // "score_val" is not a number
    std::cout << "--- Error: Type mismatch for 'score_val' ---\n";
    try {
        invoke(type_mismatch_json, greet_user, KeyArg("user_id"), KeyArg("user_name"), KeyArg("score_val"));
    }
    catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
    }
    std::cout << "\n";
    std::cin.get();
    return 0;
}