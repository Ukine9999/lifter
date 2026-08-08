#pragma once

#include <cstdint>
#include <string>

namespace Lifter::Test
{
    class TransformFixture final
    {
    public:
        explicit TransformFixture(const std::string& path);
        ~TransformFixture();

        TransformFixture(const TransformFixture&) = delete;
        TransformFixture& operator=(const TransformFixture&) = delete;

        std::uint64_t FunctionRva() const;
        std::uint64_t Evaluate(const void* input, std::uint64_t length) const;

    private:
        void* m_libraryHandle = nullptr;
        std::uint64_t (*m_transformFunction)(const void*, std::uint64_t, std::uint64_t, std::uint64_t) = nullptr;
        std::uint64_t m_functionRva = 0;
    };
}
