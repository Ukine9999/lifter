#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "Lifter/AddressSpace/FileImageProvider.hpp"
#include "Lifter/Backend/LlvmOptimizer.hpp"
#include "Lifter/Backend/LlvmRecompileBackend.hpp"
#include "Lifter/Cli/CliApplication.hpp"
#include "Lifter/Cli/CliOptions.hpp"
#include "Lifter/Cli/ConfigResolver.hpp"
#include "Lifter/Config/ConfigError.hpp"
#include "Lifter/Config/TomlConfigLoader.hpp"
#include "Lifter/Disasm/ZydisDisassembler.hpp"
#include "Lifter/Lift/X86ToLlvmLifter.hpp"
#include "Lifter/Pipeline/BinaryProcessor.hpp"
#include "Support/TransformFixture.hpp"

using namespace Lifter;

namespace
{
    Pipeline::BinaryProcessor MakeProcessor()
    {
        return Pipeline::BinaryProcessor(
            std::make_shared<AddressSpace::FileImageProvider>(), std::make_shared<Disasm::ZydisDisassembler>(),
            std::make_shared<Lift::X86ToLlvmLifter>(), std::make_shared<Backend::LlvmOptimizer>(),
            std::make_shared<Backend::LlvmRecompileBackend>());
    }
}

TEST_CASE("TOML config parses entry, file source, and output", "[config]")
{
    const Config::TomlConfigLoader loader;
    const std::string text = "entry = \"0x1000\"\n"
                             "name = \"transform_core\"\n"
                             "[[input.source]]\n"
                             "kind = \"file\"\n"
                             "path = \"target.exe\"\n"
                             "[output]\n"
                             "mode = \"compile\"\n"
                             "path = \"out.dll\"\n";

    const Config::Configuration configuration = loader.ParseText(text);

    CHECK(configuration.entry == 0x1000ull);
    CHECK(configuration.functionName == "transform_core");
    REQUIRE(configuration.sources.size() == 1);
    CHECK(configuration.sources[0].kind == Config::ESourceKind::FILE);
    CHECK(configuration.sources[0].path == "target.exe");
    CHECK(configuration.output.mode == Config::EOutputMode::COMPILE);
    CHECK(configuration.output.path == "out.dll");
}

TEST_CASE("unsupported source kinds and incomplete inputs fail with a typed error", "[config]")
{
    const Config::TomlConfigLoader loader;
    CHECK_THROWS_AS(loader.ParseText("[[input.source]]\nkind = \"registry\"\n"), Config::ConfigError);
    CHECK_THROWS_AS(loader.ParseText("[[input.source]]\nkind = \"dump\"\npath = \"process.dmp\"\n"),
                    Config::ConfigError);
    CHECK_THROWS_AS(loader.ParseText("entry = \"0x10junk\"\n"), Config::ConfigError);
    CHECK_THROWS_AS(loader.ParseText("entry = -1\n"), Config::ConfigError);
    CHECK_THROWS_AS(loader.ParseText("entry = \"18446744073709551616\"\n"), Config::ConfigError);

    Config::Configuration empty;
    CHECK_THROWS_AS(Cli::RequestFromConfiguration(empty), Config::ConfigError);
}

TEST_CASE("flag options map onto a config that yields the same request", "[config]")
{
    Cli::CliOptions options;
    options.binaryPath = "target.exe";
    options.functionRva = 0x1234;
    options.functionName = "transform_core";
    options.compile = true;
    options.outputPath = "out.dll";

    const Pipeline::ProcessRequest request = Cli::RequestFromConfiguration(Cli::ConfigurationFromOptions(options));

    CHECK(request.binaryPath == "target.exe");
    CHECK(request.functionRva == 0x1234ull);
    CHECK(request.functionName == "transform_core");
    CHECK(request.compile == true);
    CHECK(request.outputPath == "out.dll");
}

TEST_CASE("config-driven pipeline reproduces the clean transform_core lift", "[config]")
{
    const Test::TransformFixture original(LIFTER_TRANSFORM_PATH);
    Config::Configuration configuration;
    configuration.entry = original.FunctionRva();
    configuration.functionName = "TransformCore";
    Config::InputSource source;
    source.kind = Config::ESourceKind::FILE;
    source.path = LIFTER_TRANSFORM_PATH;
    configuration.sources.push_back(source);
    configuration.output.mode = Config::EOutputMode::COMPILE;
    configuration.output.path = "config_recovered.dll";

    const Pipeline::ProcessRequest request = Cli::RequestFromConfiguration(configuration);
    const Pipeline::BinaryProcessor processor = MakeProcessor();
    const Pipeline::ProcessResult result = processor.Process(request);
    REQUIRE(result.succeeded);
    REQUIRE(result.producedPath == "config_recovered.dll");

    const Test::TransformFixture recovered("config_recovered.dll");

    CHECK(recovered.Evaluate("hello", 5) == original.Evaluate("hello", 5));
    CHECK(recovered.Evaluate("a", 1) == original.Evaluate("a", 1));
    CHECK(recovered.Evaluate("", 0) == original.Evaluate("", 0));
}

TEST_CASE("CLI reports an output path that cannot be opened", "[cli]")
{
    const Test::TransformFixture original(LIFTER_TRANSFORM_PATH);
    const Pipeline::BinaryProcessor processor = MakeProcessor();
    const Config::TomlConfigLoader loader;
    std::ostringstream output;
    std::ostringstream error;
    const Cli::CliApplication application(processor, loader, output, error);
    const std::string invalidOutputPath(1, '\0');
    const std::vector<std::string> arguments = {LIFTER_TRANSFORM_PATH,
                                                "--name",
                                                "TransformCore",
                                                "--function",
                                                std::to_string(original.FunctionRva()),
                                                "--output",
                                                invalidOutputPath};

    CHECK(application.Run(arguments) == 4);
    CHECK(error.str().find("failed to open output") != std::string::npos);
    CHECK(output.str().empty());
}

TEST_CASE("pipeline rejects output paths that alias the input binary", "[pipeline]")
{
    const Test::TransformFixture original(LIFTER_TRANSFORM_PATH);
    const Pipeline::BinaryProcessor processor = MakeProcessor();

    Pipeline::ProcessRequest outputCollision;
    outputCollision.binaryPath = LIFTER_TRANSFORM_PATH;
    outputCollision.outputPath = LIFTER_TRANSFORM_PATH;
    outputCollision.functionRva = original.FunctionRva();
    outputCollision.functionName = "TransformCore";
    outputCollision.compile = true;

    const Pipeline::ProcessResult outputResult = processor.Process(outputCollision);
    CHECK_FALSE(outputResult.succeeded);
    CHECK(outputResult.diagnostics.find("must differ") != std::string::npos);

    Pipeline::ProcessRequest defaultOutputCollision = outputCollision;
    defaultOutputCollision.binaryPath = "recovered.dll";
    defaultOutputCollision.outputPath.clear();

    const Pipeline::ProcessResult defaultOutputResult = processor.Process(defaultOutputCollision);
    CHECK_FALSE(defaultOutputResult.succeeded);
    CHECK(defaultOutputResult.diagnostics.find("must differ") != std::string::npos);

    Pipeline::ProcessRequest intermediateCollision = outputCollision;
    intermediateCollision.binaryPath = "recovered.dll.obj";
    intermediateCollision.outputPath.clear();

    const Pipeline::ProcessResult intermediateResult = processor.Process(intermediateCollision);
    CHECK_FALSE(intermediateResult.succeeded);
    CHECK(intermediateResult.diagnostics.find("must differ") != std::string::npos);
}
