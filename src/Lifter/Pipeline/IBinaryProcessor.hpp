#pragma once

#include <cstdint>
#include <string>

namespace Lifter::Pipeline
{
    struct ProcessRequest
    {
        std::string binaryPath;
        std::string outputPath;
        std::uint64_t functionRva = 0;
        std::string functionName;
        bool compile = false;
    };

    struct ProcessResult
    {
        bool succeeded = false;
        std::string diagnostics;
        std::string irText;
        std::string producedPath;
    };

    class IBinaryProcessor
    {
    public:
        virtual ~IBinaryProcessor() = default;

        virtual ProcessResult Process(const ProcessRequest& request) const = 0;
    };
}
