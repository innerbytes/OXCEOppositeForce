#include "Fixtures.h"
#include "Savegame/MissionSite.h"
#include "Savegame/Waypoint.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OppositeForce;
using namespace OppositeForce::Tests;

TEST_CASE("BaseDetect_HiddenUfo_ReturnsNoneWithoutConsumingRandomness", "[hidden-ufo][detection]")
{
    // Arrange
    const bool tracked = GENERATE(false, true);
    const bool hyperwave = GENERATE(false, true);
    CAPTURE(tracked, hyperwave);
    DetectionFixture fixture("isHidden: true");
    fixture.addRadar(hyperwave);
    const auto seed = RNG::getSeed();

    // Act
    const auto detection = fixture.base.getBase().detect(&fixture.getUfo(), &fixture.save, tracked);

    // Assert
    REQUIRE(detection == DETECTION_NONE);
    REQUIRE(RNG::getSeed() == seed);
}

TEST_CASE("CraftDetect_HiddenUfoWithGuaranteedRadar_ReturnsNoneWithoutConsumingRandomness", "[hidden-ufo][detection]")
{
    // Arrange
    const bool tracked = GENERATE(false, true);
    CAPTURE(tracked);
    DetectionFixture fixture("isHidden: true");
    const auto seed = RNG::getSeed();

    // Act
    const auto detection = fixture.getCraft().detect(&fixture.getUfo(), &fixture.save, tracked);

    // Assert
    REQUIRE(detection == DETECTION_NONE);
    REQUIRE(RNG::getSeed() == seed);
}

TEST_CASE("BaseDetect_VisibleUfoWithRadar_ReturnsConfiguredDetectionType", "[hidden-ufo][detection]")
{
    // Arrange
    const auto [hyperwave, expected] = GENERATE(table<bool, UfoDetection>({
        {false, DETECTION_RADAR},
        {true, DETECTION_HYPERWAVE},
    }));
    const bool tracked = GENERATE(false, true);
    DetectionFixture fixture("size: STR_MEDIUM");
    fixture.addRadar(hyperwave);

    // Act
    const auto detection = fixture.base.getBase().detect(&fixture.getUfo(), &fixture.save, tracked);

    // Assert
    REQUIRE(detection == expected);
}

TEST_CASE("CraftDetect_VisibleUfoWithGuaranteedRadar_ReturnsRadarDetection", "[hidden-ufo][detection]")
{
    // Arrange
    const bool tracked = GENERATE(false, true);
    DetectionFixture fixture("isHidden: false");

    // Act
    const auto detection = fixture.getCraft().detect(&fixture.getUfo(), &fixture.save, tracked);

    // Assert
    REQUIRE(detection == DETECTION_RADAR);
}

TEST_CASE("BaseDetect_ForcedDetectionScript_RespectsHiddenRestriction", "[hidden-ufo][scripts]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, UfoDetection>({
        {"isHidden: true\n", DETECTION_NONE},
        {"isHidden: false\n", DETECTION_HYPERWAVE},
    }));
    const bool tracked = GENERATE(false, true);
    DetectionFixture fixture(std::string(yaml) + getForcedDetectionScripts());

    // Act
    const auto detection = fixture.base.getBase().detect(&fixture.getUfo(), &fixture.save, tracked);

    // Assert
    REQUIRE(detection == expected);
}

TEST_CASE("CraftDetect_ForcedDetectionScript_RespectsHiddenRestriction", "[hidden-ufo][scripts]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, UfoDetection>({
        {"isHidden: true\n", DETECTION_NONE},
        {"isHidden: false\n", DETECTION_HYPERWAVE},
    }));
    const bool tracked = GENERATE(false, true);
    DetectionFixture fixture(std::string(yaml) + getForcedDetectionScripts());

    // Act
    const auto detection = fixture.getCraft().detect(&fixture.getUfo(), &fixture.save, tracked);

    // Assert
    REQUIRE(detection == expected);
}

TEST_CASE("AlienMission_SpawnSiteFromHiddenUfo_CreatesSiteAtRequestedCoordinates", "[hidden-ufo][mission]")
{
    // Arrange
    DetectionFixture fixture("isHidden: true");
    auto &deployment = fixture.base.rules.createDeployment("SITE", "markerName: STR_TEST_SITE\nduration: [2, 2]");
    auto &rule = fixture.base.rules.createAlienMission("LANDING", "siteType: SITE");
    AlienMission mission(rule);
    MissionArea area;
    area.lonMin = area.lonMax = 1.25;
    area.latMin = area.latMax = -0.5;

    // Act
    auto *site = fixture.base.rules.spawnMissionSite(mission, fixture.save, area, &fixture.getUfo());

    // Assert
    REQUIRE(site != nullptr);
    REQUIRE(fixture.save.getMissionSites()->size() == 1);
    REQUIRE(site->getLongitude() == area.lonMin);
    REQUIRE(site->getLatitude() == area.latMin);
    REQUIRE(site->getDeployment() == &deployment);
    REQUIRE(site->getSecondsRemaining() == 7200);
    REQUIRE_FALSE(fixture.getUfo().getDetected());
}

TEST_CASE("MissionSite_CreatedFromHiddenUfo_UsesIndependentDetectionAndMarker", "[hidden-ufo][mission]")
{
    // Arrange
    DetectionFixture fixture("isHidden: true");
    fixture.base.rules.createDeployment("SITE");
    auto &rule = fixture.base.rules.createAlienMission("LANDING", "siteType: SITE");
    AlienMission mission(rule);
    MissionArea area;
    area.lonMin = area.lonMax = 0;
    area.latMin = area.latMax = 0;
    auto *site = fixture.base.rules.spawnMissionSite(mission, fixture.save, area, &fixture.getUfo());

    // Act
    site->setDetected(true);

    // Assert
    REQUIRE(site->getDetected());
    REQUIRE(site->getMarker() == 5);
    REQUIRE_FALSE(fixture.getUfo().getDetected());
    REQUIRE(fixture.getUfo().getMarker() == -1);
}
