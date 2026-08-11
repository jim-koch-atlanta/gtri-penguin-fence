#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "penguin_fence/latlon.hpp"
#include "penguin_fence/mission.hpp"
#include "penguin_fence/parser.hpp"

// INPUT FORMAT ASSUMPTION:
// These tests assume parsePoint accepts one point as
//     "<lat><N|S> <lon><E|W>"      e.g.  "89.9942S 179.5000W"
// (hemisphere letter right after each number; a single space between the
// latitude and longitude). If you settle on a different delimiter/format,
// update the literals below -- the *cases* are the point, not the spelling.

namespace {

using penguin_fence::LatLon;
using penguin_fence::MissionData;
using penguin_fence::parseMission;
using penguin_fence::parseMissionFile;
using penguin_fence::parsePoint;

// A successful parse must yield (elat, elon) in signed decimal degrees.
void expectParses(std::string_view in, double elat, double elon) {
    auto r = parsePoint(in);
    ASSERT_TRUE(r.ok()) << "expected success parsing: " << in;
    const LatLon& p = std::get<LatLon>(r.output);  // LatLon != error type -> unambiguous
    EXPECT_NEAR(p.lat, elat, 1e-9) << "lat for: " << in;
    EXPECT_NEAR(p.lon, elon, 1e-9) << "lon for: " << in;
}

// A failed parse must hold the error alternative with a non-empty message.
void expectRejects(std::string_view in) {
    auto r = parsePoint(in);
    ASSERT_FALSE(r.ok()) << "expected failure parsing: " << in;
    EXPECT_FALSE(std::get<std::string>(r.output).empty()) << "no error message for: " << in;
}

}  // namespace

// --- Hemisphere sign conversion -------------------------------------------
TEST(ParsePoint, NorthAndEastArePositive) {
    expectParses("10.0 N 20.0 E", 10.0, 20.0);
}
TEST(ParsePoint, SouthNegatesLatitude)    { expectParses("10.0 S 20.0 E", -10.0, 20.0); }
TEST(ParsePoint, WestNegatesLongitude)    { expectParses("10.0 N 20.0 W", 10.0, -20.0); }
TEST(ParsePoint, SouthWestMissionPoint) {
    // Representative of the mission: near the pole, near the antimeridian.
    expectParses("89.9942 S 179.5000 W", -89.9942, -179.5);
}

// --- Boundaries (this mission lives at the extremes) ----------------------
TEST(ParsePoint, EquatorAndPrimeMeridian) { expectParses("0.0 N 0.0 E", 0.0, 0.0); }  // sign of zero: don't care
TEST(ParsePoint, SouthPole)               { expectParses("90.0 S 0.0 E", -90.0, 0.0); }
TEST(ParsePoint, AntimeridianWest)        { expectParses("0.0 N 180.0 W", 0.0, -180.0); }
TEST(ParsePoint, AntimeridianEast)        { expectParses("0.0 N 180.0 E", 0.0, 180.0); }

// --- Malformed input -> failure -------------------------------------------
TEST(ParsePoint, RejectsEmpty)               { expectRejects(""); }
TEST(ParsePoint, RejectsNonNumeric)          { expectRejects("abc N 20.0 E"); }
TEST(ParsePoint, RejectsMissingHemisphere)   { expectRejects("89.99 179.5"); }  // assumes suffix required
TEST(ParsePoint, RejectsBadSuffix)           { expectRejects("10.0 Q 20.0 E"); }
TEST(ParsePoint, RejectsLatitudeOutOfRange)  { expectRejects("91.0 N 20.0 E"); }  // exercises range check
TEST(ParsePoint, RejectsLongitudeOutOfRange) { expectRejects("10.0 N 181.0 E"); }

// -------------------------------------------------------------------------
// Design-dependent cases -- decide the contract, then add tests:
//   * lowercase suffix    "10.0n 20.0e"     -> accept or reject?
//   * wrong-axis suffix   "10.0E 20.0N"     -> reject (E on a latitude)?
//   * extra/leading/trailing whitespace     -> tolerate?
//   * bare sign, no suffix "-89.99 -179.5"  -> reject (suffix required)?
// These hinge on your decisions, so I left them for you to specify.

// --- NaN / non-finite rejection -------------------------------------------
// NaN compares false to every bound, so it must be caught explicitly
// (isnan/isfinite), not by the range check. These lock in that guard.
TEST(ParsePoint, RejectsNaNLatitude)  { expectRejects("nan N 20.0 E"); }
TEST(ParsePoint, RejectsNaNLongitude) { expectRejects("10.0 N nan E"); }
TEST(ParsePoint, RejectsInfLatitude)  { expectRejects("inf N 20.0 E"); }

// =========================================================================
// parseMission -- JSON envelope: launch point + ingress route + ROI
// =========================================================================

// A small, easy-to-verify mission: launch + 3-vertex route + 5-vertex closed ROI.
constexpr std::string_view kValidMission = R"({
  "launchPoint": "10.0 N 20.0 E",
  "ingressRoute": [
    "10.0 N 20.0 E",
    "11.0 N 21.0 E",
    "12.0 S 22.0 W"
  ],
  "regionOfInterest": [
    "1.0 N 1.0 E",
    "2.0 N 1.0 E",
    "2.0 N 2.0 E",
    "1.0 N 2.0 E",
    "1.0 N 1.0 E"
  ]
})";

TEST(ParseMission, ParsesValidMission) {
    auto r = parseMission(kValidMission);
    ASSERT_TRUE(r.ok()) << r.error();   // r.error() only evaluated when the assert fails
    const MissionData& m = r.value();

    EXPECT_NEAR(m.launchPoint.lat, 10.0, 1e-9);
    EXPECT_NEAR(m.launchPoint.lon, 20.0, 1e-9);

    ASSERT_EQ(m.ingressRoute.size(), 3u);
    EXPECT_NEAR(m.ingressRoute[2].lat, -12.0, 1e-9);   // "12.0 S" -> -12
    EXPECT_NEAR(m.ingressRoute[2].lon, -22.0, 1e-9);   // "22.0 W" -> -22

    // ROI is trusted-closed on input (no auto-close), so size == vertex count.
    ASSERT_EQ(m.regionOfInterest.size(), 5u);
    EXPECT_NEAR(m.regionOfInterest.front().lat, m.regionOfInterest.back().lat, 1e-9);
    EXPECT_NEAR(m.regionOfInterest.front().lon, m.regionOfInterest.back().lon, 1e-9);
}

TEST(ParseMission, RejectsMalformedJson) {
    EXPECT_FALSE(parseMission("{ this is not json").ok());
}

TEST(ParseMission, RejectsMissingLaunchPoint) {
    EXPECT_FALSE(parseMission(R"({ "ingressRoute": [], "regionOfInterest": [] })").ok());
}

TEST(ParseMission, RejectsNonArrayIngressRoute) {
    EXPECT_FALSE(parseMission(R"({
        "launchPoint": "10.0 N 20.0 E",
        "ingressRoute": "not an array",
        "regionOfInterest": []
    })").ok());
}

TEST(ParseMission, RejectsBadPointInRoute) {
    EXPECT_FALSE(parseMission(R"({
        "launchPoint": "10.0 N 20.0 E",
        "ingressRoute": ["10.0 N 20.0 E", "garbage", "12.0 S 22.0 W"],
        "regionOfInterest": []
    })").ok());
}

TEST(ParseMission, RejectsNonStringPoint) {
    // launchPoint as a number, not a coordinate string.
    EXPECT_FALSE(parseMission(R"({
        "launchPoint": 42,
        "ingressRoute": [],
        "regionOfInterest": []
    })").ok());
}

// =========================================================================
// parseMissionFile
// =========================================================================

TEST(ParseMissionFile, MissingFileReturnsError) {
    EXPECT_FALSE(parseMissionFile("/no/such/penguin_fence_mission.json").ok());
}

TEST(ParseMissionFile, RoundTripsThroughFile) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "penguin_fence_test_mission.json";
    {
        std::ofstream out(path);
        out << kValidMission;
    }

    auto r = parseMissionFile(path);
    std::filesystem::remove(path);      // clean up before asserting

    ASSERT_TRUE(r.ok()) << r.error();
    EXPECT_EQ(r.value().ingressRoute.size(), 3u);
}
