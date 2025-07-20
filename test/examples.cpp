#include <string>
#include <vector>
#include <iostream>
#include <functional> 
#include <nlohmann/json.hpp> 
#include "func_invoke.hpp" 

namespace js = nlohmann;

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
namespace app_types {
    // 1. Define a user-defined type
    struct UserProfile {
        int id;
        std::string username;
        std::vector<std::string> roles;
    };

    // 2. Define from_json function for UserProfile
    static void from_json(const js::json& j, UserProfile& p) {
        p.id = j.at("id").get<int>();
        p.username = j.at("username").get<std::string>();
        p.roles = j.at("roles").get<std::vector<std::string>>();
    }

} // namespace app_types

namespace {
    // Function that accepts a user-defined type 
    static void display_user_profile(app_types::UserProfile profile, bool active, const std::string& status) {
        std::cout << "--- User Profile ---\n";
        std::cout << "ID: " << profile.id << "\n";
        std::cout << "Username: " << profile.username << "\n";
        std::cout << "Roles: ";
        for (const auto& role : profile.roles) {
            std::cout << role << " ";
        }
        std::cout << "\n";
        std::cout << "Active: " << std::boolalpha << active << "\n";
        std::cout << "Status: " << status << "\n";
    }

    // Example class with methods 
    class Processor {
    public:
        void process_message(const std::string& msg, int repeat_count) const {
            for (int i = 0; i < repeat_count; ++i) {
                std::cout << "Message: " << msg << "\n";
            }
        }

        void calculate_sum(int a, int b, double factor) {
            double result = (a + b) * factor;
            std::cout << "Sum Result: " << result << "\n";
        }
    };

    // Example free function (from example_lamda_and_function.cpp)
    static void greet_user(int id, const std::string& name, double score) {
        std::cout << "Hello, " << name << "! (ID: " << id << ", Score: " << score << ")\n";
    }

#if HAS_CXX20
    static void func(func_invoke::Value<"id", int> idx, std::string name, int code) {
        std::cout << "--- Value Test ---\n";
        std::cout << "ID: " << idx.data() << "\n";
        std::cout << "Name: " << name << "\n";
        std::cout << "Code: " << code << "\n";
        code = 20;
    }
#endif
}


namespace examples {
    static void run_user_type_examples() {
        using namespace func_invoke;
        std::cout << "\n--- User-Defined Type (UserProfile) Examples ---\n";
        js::json user_data_json = {
            {"user_profile_data", {
                {"id", 101},
                {"username", "developer"},
                {"roles", {"admin", "editor"}}
            }},
            {"is_active", true}
        };

        // Example 1: Successful call with user-defined type from JSON
        std::cout << "--- Successful function call with user-defined type ---\n";
        try {
            invoke(user_data_json, display_user_profile,
                KeyArg("user_profile_data"), KeyArg("is_active"), "ONLINE");
        }
        catch (const std::exception& ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
        }
        std::cout << "\n";

        // Example 2: Error - missing nested key (e.g., "username")
        js::json missing_nested_key_json = {
            {"user_profile_data", {
                {"id", 102},
                {"roles", {"guest"}}
            }}
        };
        std::cout << "--- Error: Missing nested key 'username' ---\n";
        try {
            invoke(missing_nested_key_json, display_user_profile,
                KeyArg("user_profile_data"), false, "OFFLINE");
        }
        catch (const std::exception& ex) {
            std::cerr << "ERROR: " << ex.what() << std::endl;
        }
        std::cout << "\n";

        // Example 3: Error - type mismatch for nested key (e.g., "id" not int)
        js::json type_mismatch_nested_key_json = {
            {"user_profile_data", {
                {"id", "one"},
                {"username", "tester"},
                {"roles", {"viewer"}}
            }}
        };
        std::cout << "--- Error: Type mismatch for nested key 'id' ---\n";
        try {
            invoke(type_mismatch_nested_key_json, display_user_profile,
                KeyArg("user_profile_data"), true, "AWAY");
        }
        catch (const std::exception& ex) {
            std::cerr << "ERROR: " << ex.what() << std::endl;
        }
        std::cout << "\n";
    }
    static void run_member_function_examples() {
        using namespace func_invoke;
        std::cout << "\n--- Class Method Invocation Examples ---\n";
        Processor my_processor;
        js::json method_json = {
            {"message_text", "Hello from JSON!"},
            {"val_a", 10},
            {"val_b", 20},
            {"multiplier", 2.5}
        };

        // Example 1: Calling process_message method (message from JSON, repeat count direct)
        std::cout << "--- Calling Processor::process_message ---\n";
        try {
            invoke(method_json, &Processor::process_message, my_processor, KeyArg("message_text"), 2); // Repeat 2 times
        }
        catch (const std::exception& ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
        }
        std::cout << "\n";

        // Example 2: Calling calculate_sum method (a, b, factor from JSON)
        std::cout << "--- Calling Processor::calculate_sum (all arguments from JSON) ---\n";
        try {
            invoke(method_json, &Processor::calculate_sum, my_processor,
                KeyArg("val_a"), KeyArg("val_b"), KeyArg("multiplier"));
        }
        catch (const std::exception& ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
        }
        std::cout << "\n";

        // Example 3: Error handling - missing key for method
        js::json missing_key_method_json = { {"val_a", 5} }; // "val_b" is missing
        std::cout << "--- Error: Missing key 'val_b' for method ---\n";
        try {
            invoke(missing_key_method_json, &Processor::calculate_sum, my_processor,
                KeyArg("val_a"), KeyArg("val_b"), 1.0);
        }
        catch (const std::exception& ex) {
            std::cerr << "ERROR: " << ex.what() << std::endl;
        }
        std::cout << "\n";
    }
    static void run_lambda_and_function_examples() {
        using namespace func_invoke;
        std::cout << "\n--- Free Function and Lambda Expression Examples ---\n";
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
    }

#if HAS_CXX20
    static void run_cpp20_specific_examples() {
        using namespace func_invoke;
        std::cout << "\n--- C++20 Specific Tests (func_invoke::Value) ---\n";
       
        js::json maps{
          {"id", 1 },
          {"name", "Ivan"},
        };

        try {
            int code = 10;
            invoke(maps, func, KeyArg("name"), code);
            std::cout << "Updated Code: " << code << "\n";
        }
        catch (const std::exception& ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
        }
    }
#endif

    static int run() {
        try
        {
            std::cout << "--- Running func_invoke Tests ---\n";

            run_user_type_examples();
            run_member_function_examples();
            run_lambda_and_function_examples();

#if HAS_CXX20
            run_cpp20_specific_examples();
#endif

            return 0;
        }
        catch (const std::exception& ex) {
            std::cerr << "Exeption: " << ex.what() << std::endl;
            return 0;
        }
    }
}

int main() {
	return examples::run();
}