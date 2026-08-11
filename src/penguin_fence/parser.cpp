#include "penguin_fence/mission.hpp"
#include "penguin_fence/parser.hpp"
#include "penguin_fence/types.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using json = nlohmann::json;

namespace penguin_fence {

[[nodiscard]]
ParseResult<LatLon> parsePoint(std::string_view input) {
    std::istringstream stream(std::string{input});

    double lat;
    double lon;
    char latDirection;
    char lonDirection;

    if (!(stream >> lat >> latDirection
                 >> lon >> lonDirection))
    {
        return ParseResult<LatLon>({ .output = std::string{"Failed to parse the input string."} });
    }

    switch (latDirection)
    {
        case 'N':
        case 'n':
            break;

        case 'S':
        case 's':
            lat = -lat;
            break;

        default:
            return ParseResult<LatLon>({ .output = std::string {"Invalid latitude direction."} });
    }

    switch (lonDirection)
    {
        case 'E':
        case 'e':
            break;

        case 'W':
        case 'w':
            lon = -lon;
            break;

        default:
            return ParseResult<LatLon>({ .output = std::string {"Invalid longitude direction."} });
    }

    if (lat < -90.0 || lat > 90.0)
        return ParseResult<LatLon>({ .output = std::string {"Latitude out of range."} });

    if (std::isnan(lat)) {
        return ParseResult<LatLon>({ .output = std::string {"Latitude is not a number."} });
    }

    if (lon < -180.0 || lon > 180.0)
        return ParseResult<LatLon>({ .output = std::string {"Longitude out of range."} });

    if (std::isnan(lon)) {
        return ParseResult<LatLon>({ .output = std::string {"Longitude is not a number."} });
    }

    return ParseResult<LatLon>({ .output = LatLon{ .lat = lat, .lon = lon } });
}

[[nodiscard]]
ParseResult<MissionData> parseMission(std::string_view input) {
    try {
        const auto j = json::parse(input.begin(), input.end());

        MissionData mission;

        //
        // Launch point
        //
        {
            const auto& text = j.at("launchPoint").get_ref<const std::string&>();

            auto result = parsePoint(text);
            if (!result)
            {
                return ParseResult<MissionData>({ .output = std::string{ "Invalid launchPoint: " + result.error() } });
            }

            mission.launchPoint = result.value();
        }

        //
        // Ingress route
        //
        {
            const auto& route = j.at("ingressRoute");

            if (!route.is_array())
            {
                return ParseResult<MissionData>({ .output = std::string{ "ingressRoute must be an array" } });
            }

            mission.ingressRoute.reserve(route.size());

            for (std::size_t i = 0; i < route.size(); ++i)
            {
                const auto& text = route.at(i).get_ref<const std::string&>();

                auto result = parsePoint(text);
                if (!result)
                {
                    return ParseResult<MissionData>({ .output = std::string{ "Invalid ingressRoute[" +
                        std::to_string(i) +
                        "]: " +
                        result.error() } });
                }

                mission.ingressRoute.push_back(result.value());
            }
        }

        //
        // Region of interest
        //
        {
            const auto& roi = j.at("regionOfInterest");

            if (!roi.is_array())
            {
                return ParseResult<MissionData>({ .output = std::string{ "regionOfInterest must be an array" } });
            }

            mission.regionOfInterest.reserve(roi.size() + 1);

            for (std::size_t i = 0; i < roi.size(); ++i)
            {
                const auto& text = roi.at(i).get_ref<const std::string&>();

                auto result = parsePoint(text);
                if (!result)
                {
                    return ParseResult<MissionData>({ .output = std::string{ "Invalid regionOfInterest[" +
                        std::to_string(i) +
                        "]: " +
                        result.error() } });
                }

                mission.regionOfInterest.push_back(result.value());
            }
        }

        return ParseResult<MissionData>({ .output = std::move(mission) });
    }
    catch (const json::exception& e)
    {
        return ParseResult<MissionData>({ .output = std::string{"Invalid mission JSON: "} + e.what() });
    }
}

[[nodiscard]]
ParseResult<MissionData> parseMissionFile(const std::filesystem::path& path) {
    std::ifstream file(path);

    if (!file)
    {
        return ParseResult<MissionData>({ .output = std::string{"Failed to open mission file: " + path.string()} });
    }

    const std::string contents{
        std::istreambuf_iterator<char>{file},
        std::istreambuf_iterator<char>{}
    };

    return parseMission(contents);
}

}  // namespace penguin_fence
