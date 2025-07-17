#include <iostream>
#include <string>
#include <nlohmann/json.hpp> 
#include "func_invoke.hpp" 

namespace js = nlohmann;

// Function to extract values from nlohmann::json (repeated for example completeness)
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

int main() {
    using namespace func_invoke;

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
    std::cin.get();
    return 0;
}