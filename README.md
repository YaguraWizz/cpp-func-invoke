# func_invoke.hpp

`func_invoke.hpp` — это легковесная C++ библиотека заголовков, предназначенная для упрощения **вызова функций и методов классов с использованием данных из контейнеров**, таких как JSON-объекты, карты или другие структуры, где параметры функции доступны по ключу (имени). Она автоматически извлекает необходимые аргументы из контейнера и передает их в вызываемую функцию, используя информацию о типах аргументов функции.

## ✨ Особенности

  * **Гибкий механизм извлечения значений**: Библиотека не привязана к конкретному типу контейнера (например, JSON). Вы определяете, как извлекать значения из *вашего* контейнера, реализуя специализацию **`func_invoke::extract_value`** для вашего типа контейнера и целевых типов данных.
  * **Автоматическое извлечение аргументов**: Библиотека использует метапрограммирование для определения типов аргументов функции и извлекает соответствующие значения из предоставленного контейнера. Аргументы могут быть переданы напрямую или извлечены из контейнера по ключу с использованием **`func_invoke::KeyArg`**.
  * **Поддержка различных типов вызываемых объектов**: Работает с обычными функциями, указателями на функции, `std::function`, лямбда-выражениями и методами классов.
  * **Проверка количества аргументов во время компиляции**: Статические утверждения гарантируют, что количество предоставленных аргументов (прямых или по ключу) соответствует количеству параметров функции, предотвращая ошибки на этапе компиляции.
  * **Удобство использования**: Упрощает код, избавляя от необходимости вручную извлекать и приводить каждый аргумент.
  * **Обработка ошибок при извлечении**: Предоставляет механизмы для сообщения о распространённых ошибках при извлечении данных, таких как **отсутствие ключа** или **несоответствие типов**.

-----

## ⚠️ Важное замечание о десериализации

Библиотека `func_invoke.hpp` **не занимается самостоятельной десериализацией данных в объекты**. Она построена **поверх готовых десериализаторов**. Её основная задача — обеспечить удобный механизм связывания аргументов из произвольного контейнера с параметрами функции, используя существующие способы извлечения значений.

Для каждого типа контейнера, который вы хотите использовать с `func_invoke`, вам необходимо реализовать функцию **`func_invoke::extract_value`**. Эта функция является точкой расширения, где вы определяете логику получения данных по ключу из вашего конкретного контейнера и их преобразования в требуемый тип.

-----

## 🚀 Начало работы

### Предпосылки

  * C++17 или новее (для `std::apply`, `std::make_index_sequence`, `std::string_view` и т.д.).
  * Ваш проект должен включать заголовочный файл `func_invoke.hpp`.
  * Для работы с JSON-примерами требуется библиотека `nlohmann/json`.

### Интеграция

Просто включите `func_invoke.hpp` в ваш проект. Это библиотека только для заголовков, поэтому компиляция не требуется.

```cpp
#include "func_invoke.hpp"
```

-----

## 🛠️ Использование

Прежде чем использовать функции `invoke`, вам необходимо предоставить реализацию для **`func_invoke::extract_value`**, которая будет извлекать значения из вашего конкретного типа контейнера.

### Реализация `func_invoke::extract_value`

Вам нужно специализировать `func_invoke::extract_value` для типов, которые вы хотите извлекать, и для вашего типа контейнера.

**Рекомендация:** Объявите **одну общую шаблонную функцию** `extract_value` в пространстве имён `func_invoke` для вашего типа контейнера. Внутри этой функции вы можете делегировать фактическое получение данных вашему десериализатору, а также обрабатывать возможные ошибки.

Пример для `nlohmann::json` с обработкой ошибок:

```cpp
#include "func_invoke.hpp"
#include <nlohmann/json.hpp>
#include <string_view>
#include <typeinfo> // Для typeid(T).name()

namespace func_invoke {
    template<typename T>
    static T extract_value(value_extractor_tag<T>, const nlohmann::json& jv, std::string_view key) {
        if (!jv.is_object() || !jv.contains(key.data())) {
            throw key_not_found_error(key); // Выбрасываем исключение "ключ не найден"
        }
        const nlohmann::json& value_from_json = jv.at(key.data());
        try {
            return value_from_json.get<T>();
        } catch (const nlohmann::json::exception& ex) {
            // nlohmann::json::get() может бросать исключения при несовпадении типов
            throw type_mismatch_error(key, typeid(T).name(), ex.what()); // Выбрасываем исключение "несоответствие типа"
        }
    }
} // namespace func_invoke
```

Этот подход гарантирует, что `func_invoke` будет корректно работать со всеми типами, которые ваш десериализатор (например, `nlohmann::json::get()`) способен обрабатывать, включая примитивные типы (`int`, `double`, `std::string`) и ваши **пользовательские типы**, для которых вы определили соответствующие механизмы десериализации (например, функцию `from_json` для `nlohmann::json`).

-----

### Пример использования с пользовательским типом и `nlohmann::json`

`func_invoke` может десериализовать пользовательские типы, если для них определена функция `from_json`, следующая соглашениям `nlohmann::json` (через ADL - Argument Dependent Lookup).

```cpp
#include <iostream>
#include <string>
#include <vector> // Для std::vector
#include <nlohmann/json.hpp> 
#include "func_invoke.hpp" 

namespace js = nlohmann;

// ВАЖНО: Реализация func_invoke::extract_value для nlohmann::json (см. выше)
// ... (вставьте код extract_value из предыдущего раздела)


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

int main() {
    using namespace func_invoke; // Удобно для использования KeyArg и invoke

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
    } catch (const std::exception& ex) {
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
    } catch (const std::exception& ex) {
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
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    return 0;
}
```

-----

### Вызов обычных функций и лямбд

Этот пример демонстрирует, как вызывать обычные функции и лямбда-выражения, извлекая некоторые аргументы из JSON и передавая другие напрямую.

```cpp
#include <iostream>
#include <string>
#include <functional> // Для std::function
#include <nlohmann/json.hpp> 
#include "func_invoke.hpp" 

namespace js = nlohmann;

// ВАЖНО: Реализация func_invoke::extract_value для nlohmann::json (см. выше)
// ... (вставьте код extract_value из предыдущего раздела)

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
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Example 2: Calling a free function with mixed arguments
    js::json partial_json = { {"user_name", "Bob"} };
    std::cout << "--- Calling greet_user (ID and Score direct, Name from JSON) ---\n";
    try {
        invoke(partial_json, greet_user, 456, KeyArg("user_name"), 75.0);
    } catch (const std::exception& ex) {
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
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Example 4: Error handling - missing key
    js::json missing_key_json = { {"user_id", 789}, {"score_val", 50.0} }; // "user_name" is missing
    std::cout << "--- Error: Missing key 'user_name' ---\n";
    try {
        invoke(missing_key_json, greet_user, KeyArg("user_id"), KeyArg("user_name"), KeyArg("score_val"));
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Example 5: Error handling - type mismatch
    js::json type_mismatch_json = { {"user_id", 111}, {"user_name", "Charlie"}, {"score_val", "fifty"} }; // "score_val" is not a number
    std::cout << "--- Error: Type mismatch for 'score_val' ---\n";
    try {
        invoke(type_mismatch_json, greet_user, KeyArg("user_id"), KeyArg("user_name"), KeyArg("score_val"));
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    return 0;
}
```

-----

### Вызов методов класса

Этот пример демонстрирует, как использовать `func_invoke` для вызова методов класса. Вам нужно передать указатель на метод и объект, на котором будет вызван метод.

```cpp
#include <iostream>
#include <string>
#include <nlohmann/json.hpp> 
#include "func_invoke.hpp" 

namespace js = nlohmann;

// ВАЖНО: Реализация func_invoke::extract_value для nlohmann::json (см. выше)
// ... (вставьте код extract_value из предыдущего раздела)

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
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Example 2: Calling calculate_sum method (a, b, factor from JSON)
    std::cout << "--- Calling Processor::calculate_sum (all arguments from JSON) ---\n";
    try {
        invoke(method_json, &Processor::calculate_sum, my_processor, 
               KeyArg("val_a"), KeyArg("val_b"), KeyArg("multiplier"));
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Example 3: Error handling - missing key for method
    js::json missing_key_method_json = { {"val_a", 5} }; // "val_b" is missing
    std::cout << "--- Error: Missing key 'val_b' for method ---\n";
    try {
        invoke(missing_key_method_json, &Processor::calculate_sum, my_processor, 
               KeyArg("val_a"), KeyArg("val_b"), 1.0);
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    return 0;
}
```

-----

## 💡 Принцип работы

Библиотека использует шаблоны C++ и метапрограммирование для:

1.  **Определения сигнатуры функции**: `detail::FunctionTraits` извлекает типы аргументов функции в `std::tuple`.
2.  **Построения кортежа аргументов**: `detail::build_arg_tuple` использует список ключей (или прямые значения) и предоставленную функцию **`func_invoke::extract_value`** для извлечения каждого аргумента из контейнера (если это `KeyArg`) и формирования `std::tuple` с правильными типами.
3.  **Применения кортежа**: `std::apply` (C++17) затем используется для передачи элементов кортежа в качестве отдельных аргументов в целевую функцию или метод.

-----

## 🤝 Вклад

Приветствуются любые вклады, сообщения об ошибках и предложения по улучшению. Пожалуйста, откройте issue или создайте pull request.

-----

## 📄 Лицензия

Эта библиотека распространяется под лицензией MIT. Подробнее см. файл `LICENSE`.