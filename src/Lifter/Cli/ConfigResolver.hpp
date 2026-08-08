#pragma once

#include "Lifter/Cli/CliOptions.hpp"
#include "Lifter/Config/Configuration.hpp"
#include "Lifter/Pipeline/IBinaryProcessor.hpp"

namespace Lifter::Cli
{
    Config::Configuration ConfigurationFromOptions(const CliOptions& options);

    Pipeline::ProcessRequest RequestFromConfiguration(const Config::Configuration& configuration);
}
