#include "Support/TransformFixture.hpp"

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Lifter::Test
{
    TransformFixture::TransformFixture(const std::string& path)
    {
        const std::string absolutePath = std::filesystem::absolute(path).string();
        const HMODULE library = LoadLibraryExA(absolutePath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (library == nullptr) throw std::runtime_error("failed to load transform fixture: " + absolutePath);

        const FARPROC function = GetProcAddress(library, "TransformCore");
        if (function == nullptr)
        {
            FreeLibrary(library);
            throw std::runtime_error("transform fixture does not export TransformCore: " + absolutePath);
        }

        m_libraryHandle = library;
        m_transformFunction =
            reinterpret_cast<std::uint64_t (*)(const void*, std::uint64_t, std::uint64_t, std::uint64_t)>(function);
        m_functionRva = reinterpret_cast<std::uintptr_t>(function) - reinterpret_cast<std::uintptr_t>(library);
    }

    TransformFixture::~TransformFixture()
    {
        if (m_libraryHandle != nullptr) FreeLibrary(static_cast<HMODULE>(m_libraryHandle));
    }

    std::uint64_t TransformFixture::FunctionRva() const
    {
        return m_functionRva;
    }

    std::uint64_t TransformFixture::Evaluate(const void* input, std::uint64_t length) const
    {
        return m_transformFunction(input, length, 0, 0);
    }
}
