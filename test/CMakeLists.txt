set(EXAMPLES_SOURCE_FILE "examples.cpp")

# Executable file for C++17
add_executable(func_invoke_cpp17 ${EXAMPLES_SOURCE_FILE})
target_link_libraries(func_invoke_cpp17 PRIVATE CONAN_PKG::nlohmann_json)


# Executable file for C++20
add_executable(func_invoke_cpp20 ${EXAMPLES_SOURCE_FILE})
target_compile_features(func_invoke_cpp20 PRIVATE cxx_std_20) 
target_link_libraries(func_invoke_cpp20 PRIVATE CONAN_PKG::nlohmann_json)

