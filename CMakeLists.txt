cmake_minimum_required(VERSION 3.24)

project(Lifter
    VERSION 0.1.0
    DESCRIPTION "Experimental x86-64 PE lifting and recompilation pipeline"
    LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Lifter currently supports Windows only")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

option(LIFTER_BUILD_TESTS "Build lifter tests" ${PROJECT_IS_TOP_LEVEL})
option(LIFTER_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(lifter_compile_options INTERFACE)
add_library(Lifter::CompileOptions ALIAS lifter_compile_options)
target_compile_features(lifter_compile_options INTERFACE cxx_std_20)
if(MSVC)
    target_compile_options(lifter_compile_options INTERFACE
        /W4 /permissive- /utf-8 /EHsc /Zc:__cplusplus /Zc:preprocessor /Zc:inline
        /external:anglebrackets /external:W0)
    if(LIFTER_WARNINGS_AS_ERRORS)
        target_compile_options(lifter_compile_options INTERFACE /WX)
    endif()
else()
    target_compile_options(lifter_compile_options INTERFACE -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor)
    if(LIFTER_WARNINGS_AS_ERRORS)
        target_compile_options(lifter_compile_options INTERFACE -Werror)
    endif()
endif()

find_package(zydis CONFIG REQUIRED)
find_package(LIEF CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)
find_package(LLVM CONFIG REQUIRED)
find_package(LLD CONFIG REQUIRED)

message(STATUS "LLVM version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM package: ${LLVM_DIR}")

add_library(lifter_deps INTERFACE)
target_include_directories(lifter_deps SYSTEM INTERFACE ${LLVM_INCLUDE_DIRS} ${LLD_INCLUDE_DIRS})
separate_arguments(LIFTER_LLVM_DEFINITIONS NATIVE_COMMAND "${LLVM_DEFINITIONS}")
target_compile_options(lifter_deps INTERFACE ${LIFTER_LLVM_DEFINITIONS})
target_link_libraries(lifter_deps INTERFACE
    lldCOFF
    lldCommon
    LIEF::LIEF
    Zydis::Zydis
    tomlplusplus::tomlplusplus)

add_subdirectory(src)

if(LIFTER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
