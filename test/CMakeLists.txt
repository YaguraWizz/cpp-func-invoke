file(GLOB EXAMPLE_SOURCES "*.cpp")

foreach(example_file ${EXAMPLE_SOURCES})
    get_filename_component(example_name ${example_file} NAME_WE)
    add_executable(${example_name} ${example_file})
    target_link_libraries(${example_name} PRIVATE CONAN_PKG::nlohmann_json)
endforeach()