cmake_minimum_required(VERSION 3.15)
project(func_invoke CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin/RelWithDebInfo)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/bin/MinSizeRel)

option(BUILD_TESTS "Build tests for func_invoke library" OFF)

include_directories(func_invoke)

if(BUILD_TESTS)
    # Conan - только для тестов/примеров
    include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
    conan_basic_setup(TARGETS)
    message(STATUS "Contents of CONAN_TARGETS: ${CONAN_TARGETS}")
    
    enable_testing()
    add_subdirectory(test)
endif()


add_library(func_invoke_headers INTERFACE)
target_include_directories(func_invoke_headers INTERFACE func_invoke)