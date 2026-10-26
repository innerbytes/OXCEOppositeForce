#include <filesystem>
#include <functional>
#include <memory>

#include "Engine/FileMap.h"
#include "Engine/Game.h"
#include "Engine/Options.h"
#include "Engine/SurfaceSet.h"
#include "Fixtures.h"
#include "Geoscape/Globe.h"
#include "LandingFixtures.h"
#include "Mod/AlienRace.h"
#include "Mod/RuleGlobe.h"
#include "Mod/Texture.h"
#include "Mod/UfoTrajectory.h"
#include "OppositeForce/Battlescape/Game/GlobalTerrain.h"
#include "OppositeForce/Geoscape/Game/LandingTextures.h"
#include "Savegame/MissionSite.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

TEST_CASE("LandingTextures_ExcludedInitialArea_SelectsAllowedAreaUsingActualTexture", "[landing-textures]")
{
    // Arrange
    LandingFixture fixture;
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = true;
    int selectedArea = 0;

    // Act
    const auto position = LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, 0, ufo, fixture.rule,
                                                              failed, selectedArea);

    // Assert
    REQUIRE_FALSE(failed);
    REQUIRE(selectedArea == 1);
    REQUIRE(fixture.getTexture(position) == 1);
    REQUIRE(fixture.region.getMissionZones()[0].areas[0].texture == 0);
}

TEST_CASE("LandingTextures_MixedTextureArea_RejectsIceCoordinates", "[landing-textures]")
{
    // Arrange
    LandingFixture fixture;
    fixture.loadRegion("[[1, 12, 1, 2]]");
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = true;
    int selectedArea = 0;

    // Act
    const auto position = LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, 0, ufo, fixture.rule,
                                                              failed, selectedArea);

    // Assert
    REQUIRE_FALSE(failed);
    REQUIRE(selectedArea == 0);
    REQUIRE(fixture.getTexture(position) == 1);
}

TEST_CASE("LandingTextures_AllAreasExcluded_ReturnsFailureWithoutForcedLanding", "[landing-textures]")
{
    // Arrange
    LandingFixture fixture("excludedLandingTextures: [1, 9, 12]");
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = false;
    int selectedArea = 0;

    // Act
    LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, 0, ufo, fixture.rule, failed, selectedArea);

    // Assert
    REQUIRE(failed);
    REQUIRE(selectedArea == 0);
}

TEST_CASE("LandingTextures_PointAreas_RejectsExcludedPointAndPreservesMissionSiteArea", "[landing-textures]")
{
    // Arrange
    LandingFixture fixture;
    fixture.loadRegion("[[1, 1, 1, 1], [11, 11, 1, 1]]");
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = true;
    int selectedArea = 37;

    // Act
    const auto position = LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, -1, ufo, fixture.rule,
                                                              failed, selectedArea);

    // Assert
    REQUIRE_FALSE(failed);
    REQUIRE(selectedArea == 37);
    REQUIRE(fixture.getTexture(position) == 1);
}

TEST_CASE("LandingTextures_FakeWaterPreference_PreservesChosenSurfaceAcrossAreas", "[landing-textures]")
{
    // Arrange
    const bool wantsFakeWater = GENERATE(false, true);
    LandingFixture fixture("excludedLandingTextures: [12]");
    fixture.loadGlobe("textures: [{id: 1, fakeUnderwater: true}]");
    fixture.rules.loadRule(fixture.ufoRule, "fakeWaterLandingChance: " + std::to_string(wantsFakeWater ? 100 : 0));
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = true;
    int selectedArea = 0;

    // Act
    const auto position = LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, 0, ufo, fixture.rule,
                                                              failed, selectedArea);

    // Assert
    REQUIRE_FALSE(failed);
    REQUIRE(fixture.globe->insideFakeUnderwaterTexture(position.first, position.second) == wantsFakeWater);
    REQUIRE(selectedArea == (wantsFakeWater ? 1 : 0));
}

TEST_CASE("LandingTextures_RegionBoundary_OnlyMissionSiteLandingCanIgnoreIt", "[landing-textures]")
{
    // Arrange
    const bool forSite = GENERATE(false, true);
    LandingFixture fixture;
    fixture.loadRegion("[[11, 12, 1, 2]]", "[[0, 5, 0, 5]]");
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = false;
    int selectedArea = 0;

    // Act
    LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, forSite ? 0 : -1, ufo, fixture.rule, failed,
                                        selectedArea);

    // Assert
    REQUIRE(failed == !forSite);
}

TEST_CASE("LandingTextures_AreaFallback_RollsFakeWaterPreferenceOnlyOnce", "[landing-textures]")
{
    // Arrange
    const uint64_t seed = GENERATE(1, 2, 3, 4);
    LandingFixture fixture;
    fixture.loadRegion("[[1, 2, 1, 2], [11, 12, 1, 2], [21, 22, 1, 2]]", "[[0, 30, 0, 10]]");
    fixture.loadGlobe(
        "polygons:\n"
        "  - [9, 0, 0, 5, 0, 5, 5, 0, 5]\n"
        "  - [1, 10, 0, 15, 0, 15, 5, 10, 5]\n"
        "  - [2, 20, 0, 25, 0, 25, 5, 20, 5]\n"
        "textures: [{id: 1, fakeUnderwater: true}]");
    fixture.rules.loadRule(fixture.ufoRule, "fakeWaterLandingChance: 50");
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = true;
    int selectedArea = 0;
    RNG::setSeed(seed);
    const bool wantsFakeWater = RNG::percent(50);
    RNG::setSeed(seed);

    // Act
    LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, 0, ufo, fixture.rule, failed, selectedArea);

    // Assert
    REQUIRE_FALSE(failed);
    REQUIRE(selectedArea == (wantsFakeWater ? 1 : 2));
}
