add_library(lifter_core STATIC
    Lifter/AddressSpace/MappedAddressSpace.cpp
    Lifter/AddressSpace/FileImageProvider.cpp
    Lifter/Disasm/ControlFlowGraph.cpp
    Lifter/Disasm/ZydisDisassembler.cpp
    Lifter/Lift/X86ToLlvmLifter.cpp
    Lifter/Ir/OwnedLlvmModule.cpp
    Lifter/Backend/LlvmOptimizer.cpp
    Lifter/Backend/LlvmRecompileBackend.cpp
    Lifter/Pipeline/BinaryProcessor.cpp
    Lifter/Config/TomlConfigLoader.cpp
    Lifter/Cli/ConfigResolver.cpp
    Lifter/Cli/CliParser.cpp
    Lifter/Cli/CliApplication.cpp
    Lifter/Composition/CompositionRoot.cpp)

target_include_directories(lifter_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lifter_core
    PRIVATE Lifter::CompileOptions lifter_deps)
target_compile_definitions(lifter_core PRIVATE LIFTER_VERSION="${PROJECT_VERSION}")

add_executable(lifter Lifter/main.cpp)
target_link_libraries(lifter PRIVATE lifter_core Lifter::CompileOptions)
