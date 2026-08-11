#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <variant>

#include "latlon.hpp"
#include "mission.hpp"

namespace penguin_fence {

template <typename Output>
class ParseResult {
    public:

    /* Output on success, error message on failure. */
    std::variant<Output, std::string> output;

    explicit operator bool() const { return output.index() == 0; } // 0 = value, 1 = error

    bool ok() const { return static_cast<bool>(*this); }

    /* The parsed value -- valid only when ok(). Index-based (get<0>) so it stays
       correct even if Output is std::string; throws std::bad_variant_access if
       called on an error result, so check ok() first. */
    const Output& value() const { return std::get<0>(output); }

    /* The error message -- valid only when !ok(). */
    const std::string& error() const { return std::get<1>(output); }
};

[[nodiscard]] ParseResult<LatLon> parsePoint(std::string_view input);

[[nodiscard]] ParseResult<MissionData> parseMission(std::string_view input);

[[nodiscard]] ParseResult<MissionData> parseMissionFile(const std::filesystem::path& path);

}  // namespace penguin_fence
