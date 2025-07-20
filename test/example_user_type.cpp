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


namespace app_types {
    // 1. Define a user-defined type
    struct UserProfile {
        int id;
        std::string username;
        std::vector<std::string> roles;
    };

    // 2. Define from_json function for UserProfile
    // This allows nlohmann::json (and func_invoke) to automatically deserialize UserProfile
    static void from_json(const js::json& j, UserProfile& p) {
        p.id = j.at("id").get<int>();
        p.username = j.at("username").get<std::string>();
        p.roles = j.at("roles").get<std::vector<std::string>>();
    }
} // namespace app_types

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



static void func(func_invoke::Value<"id", int> idx, std::string name, int& code) {
    std::cout << "--- Value Test ---\n";
    std::cout << "ID: " << idx.data() << "\n";
    std::cout << "Name: " << name << "\n";
    std::cout << "Code: " << code << "\n";

    code = 20;
}



int main() {
    using namespace func_invoke;

    js::json maps {
      {"id", 1 },
      {"name", "Ivan"},
    };
    
    try {
        int code = 10;
        invoke(maps, func, KeyArg("name"), code);

        std::cout << "Update Code: " << code << "\n";
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

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
    std::cin.get();
    return 0;
}