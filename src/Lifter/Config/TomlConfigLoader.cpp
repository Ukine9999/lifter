#include "Lifter/Config/TomlConfigLoader.hpp"

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include <toml++/toml.hpp>

#include "Lifter/Config/ConfigError.hpp"

namespace Lifter::Config
{
    namespace
    {
        ESourceKind ParseSourceKind(const std::string& value)
        {
            if (value == "file") return ESourceKind::FILE;
            throw ConfigError("config: unknown input source kind '" + value + "'");
        }

        EOutputMode ParseOutputMode(const std::string& value)
        {
            if (value == "ir") return EOutputMode::IR;
            if (value == "compile") return EOutputMode::COMPILE;
            throw ConfigError("config: unknown output mode '" + value + "'");
        }

        std::string RequireString(const toml::table& table, std::string_view key, std::string_view context)
        {
            const auto value = table[key].value<std::string>();
            if (!value)
                throw ConfigError("config: " + std::string(context) + " requires string field '" + std::string(key) +
                                  "'");
            return *value;
        }

        std::uint64_t ParseEntry(const toml::node_view<const toml::node>& node)
        {
            if (node.is_string())
            {
                const std::string text = node.value<std::string>().value();
                std::string_view value = text;
                int base = 10;
                if (value.size() > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X'))
                {
                    base = 16;
                    value.remove_prefix(2);
                }

                std::uint64_t entry = 0;
                const char* const begin = value.data();
                const char* const end = value.data() + value.size();
                const std::from_chars_result result = std::from_chars(begin, end, entry, base);
                if (value.empty() || result.ec != std::errc() || result.ptr != end)
                    throw ConfigError("config: 'entry' must be a non-negative decimal or hexadecimal integer");

                return entry;
            }

            if (node.is_integer())
            {
                const std::int64_t entry = node.value<std::int64_t>().value();
                if (entry < 0) throw ConfigError("config: 'entry' must be non-negative");
                return static_cast<std::uint64_t>(entry);
            }

            throw ConfigError("config: 'entry' must be a hex/decimal string or an integer");
        }

        Configuration BuildConfiguration(const toml::table& root)
        {
            Configuration configuration;

            if (const auto entry = root["entry"]) configuration.entry = ParseEntry(entry);
            if (const auto name = root["name"].value<std::string>()) configuration.functionName = *name;
            if (const auto sources = root["input"]["source"].as_array())
                for (const auto& element : *sources)
                {
                    const toml::table* sourceTable = element.as_table();
                    if (sourceTable == nullptr) throw ConfigError("config: each [[input.source]] must be a table");

                    InputSource source;
                    source.kind = ParseSourceKind(RequireString(*sourceTable, "kind", "input.source"));
                    if (const auto path = (*sourceTable)["path"].value<std::string>()) source.path = *path;
                    configuration.sources.push_back(std::move(source));
                }

            if (const auto output = root["output"].as_table())
            {
                if (const auto mode = (*output)["mode"].value<std::string>())
                    configuration.output.mode = ParseOutputMode(*mode);
                if (const auto path = (*output)["path"].value<std::string>()) configuration.output.path = *path;
            }

            return configuration;
        }
    }

    Configuration TomlConfigLoader::ParseText(const std::string& text) const
    {
        try
        {
            return BuildConfiguration(toml::parse(text));
        }
        catch (const toml::parse_error& error)
        {
            throw ConfigError(std::string("config: TOML parse error: ") + error.description().data());
        }
    }

    Configuration TomlConfigLoader::LoadFile(const std::string& path) const
    {
        try
        {
            return BuildConfiguration(toml::parse_file(path));
        }
        catch (const toml::parse_error& error)
        {
            throw ConfigError("config: TOML parse error in " + path + ": " + std::string(error.description()));
        }
    }
}
