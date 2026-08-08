#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

#include "Lifter/Cli/CliCommand.hpp"
#include "Lifter/Cli/CliParser.hpp"

using Lifter::Cli::CliCommand;
using Lifter::Cli::CliParseOutcome;
using Lifter::Cli::CliParser;

namespace
{
    CliParseOutcome Parse(std::vector<std::string> arguments)
    {
        return CliParser{}.Parse(arguments);
    }
}

TEST_CASE("help and version flags are recognized", "[cli]")
{
    CHECK(Parse({"--help"}).options.command == CliCommand::ShowHelp);
    CHECK(Parse({"-h"}).options.command == CliCommand::ShowHelp);
    CHECK(Parse({"--version"}).options.command == CliCommand::ShowVersion);
}

TEST_CASE("no arguments is rejected with a message", "[cli]")
{
    const CliParseOutcome outcome = Parse({});

    CHECK_FALSE(outcome.succeeded);
    CHECK_FALSE(outcome.errorMessage.empty());
}

TEST_CASE("a bare path selects binary processing", "[cli]")
{
    const CliParseOutcome outcome = Parse({"sample.exe"});

    REQUIRE(outcome.succeeded);
    CHECK(outcome.options.command == CliCommand::ProcessBinary);
    CHECK(outcome.options.binaryPath == "sample.exe");
}

TEST_CASE("the function option parses hex and decimal relative addresses", "[cli]")
{
    CHECK(Parse({"sample.exe", "--function", "0x1000"}).options.functionRva == 0x1000u);
    CHECK(Parse({"sample.exe", "--function", "4096"}).options.functionRva == 4096u);
}

TEST_CASE("without an explicit function the entry is unset and no name is assumed", "[cli]")
{
    const CliParseOutcome outcome = Parse({"sample.exe"});

    REQUIRE(outcome.succeeded);
    CHECK(outcome.options.functionRva == 0u);
    CHECK(outcome.options.functionName.empty());
}

TEST_CASE("the name option overrides the recovered symbol name", "[cli]")
{
    const CliParseOutcome outcome = Parse({"sample.exe", "--name", "my_hash"});

    REQUIRE(outcome.succeeded);
    CHECK(outcome.options.functionName == "my_hash");
}

TEST_CASE("the compile flag and output option are captured together", "[cli]")
{
    const CliParseOutcome outcome = Parse({"sample.exe", "--compile", "-o", "out.dll"});

    REQUIRE(outcome.succeeded);
    CHECK(outcome.options.compile);
    CHECK(outcome.options.outputPath == "out.dll");
}

TEST_CASE("a missing function value is rejected", "[cli]")
{
    CHECK_FALSE(Parse({"sample.exe", "--function"}).succeeded);
}

TEST_CASE("a non-numeric --function value is rejected", "[cli]")
{
    CHECK_FALSE(Parse({"sample.exe", "--function", "notanumber"}).succeeded);
}

TEST_CASE("a trailing-garbage --function value is rejected", "[cli]")
{
    CHECK_FALSE(Parse({"sample.exe", "--function", "0x10zz"}).succeeded);
}

TEST_CASE("unknown options are rejected", "[cli]")
{
    CHECK_FALSE(Parse({"sample.exe", "--frobnicate"}).succeeded);
}

TEST_CASE("discovery commands reject extra operands and pipeline flags", "[cli]")
{
    CHECK_FALSE(Parse({"--help", "sample.exe"}).succeeded);
    CHECK_FALSE(Parse({"--version", "--compile"}).succeeded);
}

TEST_CASE("two input binaries are rejected", "[cli]")
{
    CHECK_FALSE(Parse({"a.exe", "b.exe"}).succeeded);
}

TEST_CASE("config input rejects direct processing options", "[cli]")
{
    CHECK_FALSE(Parse({"--config", "lifter.toml", "--name", "TransformCore"}).succeeded);
    CHECK_FALSE(Parse({"--config", "lifter.toml", "--function", "0x1000"}).succeeded);
    CHECK_FALSE(Parse({"--config", "lifter.toml", "--compile"}).succeeded);
    CHECK_FALSE(Parse({"--config", "lifter.toml", "--output", "out.dll"}).succeeded);
}
