find_package(Catch2 3 CONFIG REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${Catch2_DIR})
include(Catch)

add_subdirectory(${PROJECT_SOURCE_DIR}/samples/Transform ${PROJECT_BINARY_DIR}/samples/Transform)

add_executable(lifter_tests
    Support/TransformFixture.cpp
    Unit/CliParserTests.cpp
    Unit/ImageLoaderTests.cpp
    Unit/DisassemblerTests.cpp
    Unit/LifterTests.cpp
    Unit/EndToEndTests.cpp
    Unit/ConfigTests.cpp)
target_include_directories(lifter_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lifter_tests PRIVATE
    lifter_core
    Catch2::Catch2WithMain
    Lifter::CompileOptions)
add_dependencies(lifter_tests LifterTransformFixture)
target_compile_definitions(lifter_tests PRIVATE LIFTER_TRANSFORM_PATH="$<TARGET_FILE:LifterTransformFixture>")
catch_discover_tests(lifter_tests WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
