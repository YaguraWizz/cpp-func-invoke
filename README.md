# func_invoke.hpp

`func_invoke.hpp` — это легковесная C++ библиотека заголовков, предназначенная для упрощения **вызова функций и методов классов с использованием данных из контейнеров**, таких как JSON-объекты, карты или другие структуры, где параметры функции доступны по ключу (имени). Она автоматически извлекает необходимые аргументы из контейнера и передает их в вызываемую функцию, используя информацию о типах аргументов функции.

## ✨ Особенности

  * **Гибкий механизм извлечения значений**: Библиотека не привязана к конкретному типу контейнера (например, JSON). Вы определяете, как извлекать значения из *вашего* контейнера, реализуя специализацию **`func_invoke::extract_value`** для вашего типа контейнера и целевых типов данных.
  * **Автоматическое извлечение аргументов**: Библиотека использует метапрограммирование для определения типов аргументов функции и извлекает соответствующие значения из предоставленного контейнера.
  * **Поддержка различных типов вызываемых объектов**: Работает с обычными функциями, указателями на функции, `std::function`, лямбда-выражениями и методами классов.
  * **Проверка количества аргументов во время компиляции**: Статические утверждения гарантируют, что количество предоставленных ключей соответствует количеству аргументов функции, предотвращая ошибки на этапе компиляции.
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

**Рекомендация:** Объявите **одну общую шаблонную функцию** `extract_value` в пространстве имён `func_invoke` для вашего типа контейнера. Внутри этой функции вы можете делегировать фактическое получение данных вашему десериализатору, а также обрабатывать возможные ошибки. Это позволит ADL (Argument-Dependent Lookup) найти способ получения стандартных примитивных типов и ваших пользовательских типов, для которых ваш десериализатор уже имеет обработчики.

Пример для `boost::json::value` с обработкой ошибок:

```cpp
#include <boost/json.hpp>
#include <string_view>
#include <typeinfo>

namespace func_invoke {
    template<typename T>
    static T extract_value(value_extractor_tag<T>, const boost::json::value& jv, std::string_view key) {
        // 1. Проверка существования ключа
        if (!jv.is_object() || !jv.as_object().contains(key.data())) {
            throw key_not_found_error(key); // Выбрасываем исключение "ключ не найден"
        }

        const boost::json::value& value_from_json = jv.at(key.data());

        // 2. Попытка преобразования типа с отловом boost::json::error
        try {
            return boost::json::value_to<T>(value_from_json);
        }
        catch (const std::exception& ex) {
            // boost::json::value_to может бросать system_error или другие исключения при несовпадении типов
            throw type_mismatch_error(key, typeid(T).name()); // Выбрасываем исключение "несоответствие типа"
        }
    }
} // namespace func_invoke
```

Этот подход гарантирует, что `func_invoke` будет корректно работать со всеми типами, которые ваш десериализатор (например, `boost::json::value_to`) способен обрабатывать, включая примитивные типы (`int`, `double`, `std::string`) и ваши **пользовательские типы**, для которых вы определили соответствующие механизмы десериализации (например, `tag_invoke` для Boost.JSON). Включение обработки исключений `key_not_found_error` и `type_mismatch_error` делает процесс извлечения данных более надёжным и информативным при возникновении проблем.

-----

### Пример использования с пользовательским типом и `boost::json`

Для того чтобы `boost::json::value_to` мог десериализовать ваш пользовательский тип, вам нужно определить функцию `tag_invoke` для него:

```cpp
#include <iostream>
#include <boost/json.hpp>
#include "func_invoke.hpp"

namespace js = boost::json;

namespace test {
    // Пример пользовательского типа
    struct MyCustomType {
        int value;
        std::string name;
    };

    // Определение tag_invoke для конвертации boost::json::value в MyCustomType
    // Это часть механизма десериализации boost::json
    static MyCustomType tag_invoke(js::value_to_tag<MyCustomType>, const js::value& jv) {
        const js::object& obj_json = jv.as_object();
        // Здесь также могут возникать исключения Boost.JSON, если ключи отсутствуют или типы не совпадают
        return { js::value_to<int>(obj_json.at("value1")), js::value_to<std::string>(obj_json.at("name")) };
    }
} // namespace test

// ВАЖНО: Реализация func_invoke::extract_value для boost::json::value (см. выше)
// Эта функция использует tag_invoke для пользовательских типов.
// ... (вставьте код extract_value из предыдущего раздела)

static void process_custom_type(test::MyCustomType data) {
    std::cout << "MyCustomType -> value: " << data.value << ", name: " << data.name << "\n";
}

int main() {
    js::value j5 = {
        {"my_data", {{"value1", 100}, {"name", "Custom Data"}}}
    };
    js::value j6_missing_key = {
        {"my_data", {{"value1", 100}}} // Отсутствует ключ "name"
    };
    js::value j7_type_mismatch = {
        {"my_data", {{"value1", "not_an_int"}, {"name", "Custom Data"}}} // Несоответствие типа для "value1"
    };
    js::value j8_wrong_root_key = {
        {"wrong_key", {{"value1", 100}, {"name", "Custom Data"}}} // Неверный корневой ключ
    };

    // Вызов функции с пользовательским типом (успешный случай)
    std::cout << "--- Успешный вызов ---\n";
    try {
        func_invoke::invoke(j5, process_custom_type, { "my_data" }); // Выведет: MyCustomType -> value: 100, name: Custom Data
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Пример обработки исключения: Отсутствует вложенный ключ (обрабатывается tag_invoke Boost.JSON)
    std::cout << "--- Отсутствует вложенный ключ (обрабатывается Boost.JSON) ---\n";
    try {
        func_invoke::invoke(j6_missing_key, process_custom_type, { "my_data" });
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Пример обработки исключения: Несоответствие типа для вложенного ключа (обрабатывается Boost.JSON)
    std::cout << "--- Несоответствие типа для вложенного ключа (обрабатывается Boost.JSON) ---\n";
    try {
        func_invoke::invoke(j7_type_mismatch, process_custom_type, { "my_data" });
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Пример обработки исключения: Корневой ключ не найден (обрабатывается func_invoke::extract_value)
    std::cout << "--- Корневой ключ не найден (обрабатывается func_invoke) ---\n";
    try {
        func_invoke::invoke(j8_wrong_root_key, process_custom_type, { "my_data" });
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl; // Выведет: Key not found: my_data
    }
    std::cout << "\n";

    return 0;
}
```

-----

### Вызов обычных функций

Используйте **`func_invoke::invoke`** для вызова обычных функций, лямбда-выражений или `std::function`. Когда вы передаёте имя обычной функции, оно **неявно преобразуется в указатель на эту функцию**.

```cpp
#include "func_invoke.hpp"
#include <iostream>
#include <boost/json.hpp> 



namespace js = boost::json;

// ВАЖНО: Реализация func_invoke::extract_value для boost::json::value (см. выше)
// Эта функция использует tag_invoke для пользовательских типов.
// ... (вставьте код extract_value из предыдущего раздела)

void funcA(int id, double val, const std::string& name) {
    std::cout << "funcA -> id: " << id << ", val: " << val << ", name: " << name << "\n";
}

int main() {
    js::value j1 = { {"id", 42}, {"val", 3.14}, {"name", "Bob"} };
    js::value j1_missing_id = { {"val", 3.14}, {"name", "Bob"} };
    js::value j1_type_mismatch_val = { {"id", 42}, {"val", "three point one four"}, {"name", "Bob"} };

    const std::string_view keys_funcA[] = { "id", "val", "name" };

    // Успешный вызов
    std::cout << "--- Успешный вызов funcA ---\n";
    try {
        func_invoke::invoke(j1, funcA, keys_funcA); // Выведет: funcA -> id: 42, val: 3.14, name: Bob
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Пример обработки исключения: Ключ 'id' не найден
    std::cout << "--- Отсутствует ключ 'id' ---\n";
    try {
        func_invoke::invoke(j1_missing_id, funcA, keys_funcA);
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl; // Выведет: Key not found: id
    }
    std::cout << "\n";

    // Пример обработки исключения: Несоответствие типа для 'val'
    std::cout << "--- Несоответствие типа для 'val' ---\n";
    try {
        func_invoke::invoke(j1_type_mismatch_val, funcA, keys_funcA);
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl; // Выведет: Type mismatch for key 'val'. Expected type: double (или аналогично)
    }
    std::cout << "\n";


    auto lambda = [](int a, float b) {
        std::cout << "lambda -> a: " << a << ", b: " << b << "\n";
    };
    js::value j2 = { {"a", 10}, {"b", 2.5} };
    const std::string_view keys_lambda[] = { "a", "b" };
    
    // Успешный вызов лямбды
    std::cout << "--- Успешный вызов лямбды ---\n";
    try {
        func_invoke::invoke(j2, lambda, keys_lambda); // Выведет: lambda -> a: 10, b: 2.5
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    std::function<void(std::string)> f = [](std::string msg) {
        std::cout << "std::function -> " << msg << "\n";
    };
    js::value j3 = { {"msg", "hi from std::function"} };
    const std::string_view keys_f[] = { "msg" };

    // Успешный вызов std::function
    std::cout << "--- Успешный вызов std::function ---\n";
    try {
        func_invoke::invoke(j3, f, keys_f); // Выведет: std::function -> hi from std::function
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    return 0;
}
```

-----

### Вызов методов класса

Вы можете использовать **`func_invoke::invoke`** для вызова методов класса.

```cpp
#include "func_invoke.hpp"
#include <iostream>
#include <boost/json.hpp> 


namespace js = boost::json;

// ВАЖНО: Реализация func_invoke::extract_value для boost::json::value (см. выше)
// Эта функция использует tag_invoke для пользовательских типов.
// ... (вставьте код extract_value из предыдущего раздела)

class Handler {
public:
    void greet(const std::string& name) const {
        std::cout << "Hello, " << name << "!\n";
    }
};

int main() {
    Handler h;
    js::value j4 = { {"name", "Alice"} };
    js::value j4_missing_name = { {"age", 30} };

    const std::string_view keys_greet[] = { "name" };

    // Успешный вызов метода
    std::cout << "--- Успешный вызов метода ---\n";
    try {
        func_invoke::invoke(j4, &Handler::greet, h, keys_greet); // Выведет: Hello, Alice!
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl;
    }
    std::cout << "\n";

    // Пример обработки исключения: Ключ 'name' не найден
    std::cout << "--- Отсутствует ключ 'name' для метода ---\n";
    try {
        func_invoke::invoke(j4_missing_name, &Handler::greet, h, keys_greet);
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << std::endl; // Выведет: Key not found: name
    }
    std::cout << "\n";

    return 0;
}
```

-----

## 💡 Принцип работы

Библиотека использует шаблоны C++ и метапрограммирование для:

1.  **Определения сигнатуры функции**: `detail::FunctionTraits` извлекает типы аргументов функции в `std::tuple`.
2.  **Построения кортежа аргументов**: `detail::build_arg_tuple` использует список ключей и предоставленную функцию **`func_invoke::extract_value`** для извлечения каждого аргумента из контейнера и формирования `std::tuple` с правильными типами.
3.  **Применения кортежа**: `std::apply` (C++17) затем используется для передачи элементов кортежа в качестве отдельных аргументов в целевую функцию или метод.

-----

## 🤝 Вклад

Приветствуются любые вклады, сообщения об ошибках и предложения по улучшению. Пожалуйста, откройте issue или создайте pull request.

-----

## 📄 Лицензия

Эта библиотека распространяется под лицензией MIT. Подробнее см. файл `LICENSE`.