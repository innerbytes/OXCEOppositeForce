#include <algorithm>

#include "Engine/Exception.h"
#include "Fixtures.h"
#include "GlobalTerrainFixtures.h"
#include "Mod/MapScript.h"
#include "Mod/Texture.h"
#include "OppositeForce/Battlescape/Game/GlobalTerrain.h"
#include "Savegame/MissionSite.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

TEST_CASE("GlobalTerrain_DifferentSiteAndGlobeTextures_UsesGeographyOnlyWhenEnabled", "[global-terrain][selection]")
{
    // Arrange
    const bool enabled = GENERATE(false, true);
    CAPTURE(enabled);
    GlobalTerrainFixture fixture(enabled ? "useGlobalTerrain: true" : "{}");
    const auto seed = RNG::getSeed();

    // Act
    auto *selected = GlobalTerrain::selectTerrain(fixture.rules.getMod(), fixture.deployment, nullptr,
                                                  &fixture.globeTexture, &fixture.site);
    const auto seedAfterHook = RNG::getSeed();
    // A null result leaves the generator's existing site-texture selection in control.
    auto *resolved = selected
                         ? selected
                         : fixture.rules.getMod().getTerrain(fixture.siteTexture.getRandomTerrain(&fixture.site), true);

    // Assert
    REQUIRE(resolved == (enabled ? &fixture.forest : &fixture.urban));
    REQUIRE((selected != nullptr) == enabled);
    if (!enabled) REQUIRE(seedAfterHook == seed);
    REQUIRE(fixture.site.getTexture() == 1);
    REQUIRE(fixture.siteTexture.getStartingCondition() == "SITE_CONDITION");
}

TEST_CASE("GlobalTerrain_ExplicitDeploymentTerrains_DefersToExistingSelectionWithoutConsumingRandomness",
          "[global-terrain][selection]")
{
    // Arrange
    GlobalTerrainFixture fixture("useGlobalTerrain: true\nterrains: [URBAN, FOREST]");
    loadTexture(fixture.globeTexture, fixture.rules.getMod(), "terrain: [{name: UNKNOWN}]");
    const auto seed = RNG::getSeed();

    // Act
    const auto *selected = GlobalTerrain::selectTerrain(fixture.rules.getMod(), fixture.deployment, nullptr,
                                                        &fixture.globeTexture, &fixture.site);

    // Assert
    REQUIRE(selected == nullptr);
    REQUIRE(fixture.deployment.getTerrains() == std::vector<std::string>{"URBAN", "FOREST"});
    REQUIRE(RNG::getSeed() == seed);
}

TEST_CASE("GlobalTerrain_AlreadySelectedTerrain_PreservesChoiceWithoutConsumingRandomness",
          "[global-terrain][selection]")
{
    // Arrange
    GlobalTerrainFixture fixture;
    loadTexture(fixture.globeTexture, fixture.rules.getMod(), "terrain: [{name: UNKNOWN}]");
    const auto seed = RNG::getSeed();

    // Act
    const auto *selected = GlobalTerrain::selectTerrain(fixture.rules.getMod(), fixture.deployment, &fixture.urban,
                                                        &fixture.globeTexture, &fixture.site);

    // Assert
    REQUIRE(selected == &fixture.urban);
    REQUIRE(RNG::getSeed() == seed);
}

TEST_CASE("GlobalTerrain_MissingOrEmptyGlobeTexture_UsesFirstAvailableTerrain", "[global-terrain][selection]")
{
    // Arrange
    GlobalTerrainFixture fixture;
    const bool missing = GENERATE(false, true);
    CAPTURE(missing);
    Texture empty(3);
    const auto seed = RNG::getSeed();

    // Act
    const auto *selected = GlobalTerrain::selectTerrain(fixture.rules.getMod(), fixture.deployment, nullptr,
                                                        missing ? nullptr : &empty, &fixture.site);

    // Assert
    REQUIRE(selected == &fixture.urban);
    REQUIRE(selected->getName() == fixture.rules.getMod().getTerrainList().front());
    REQUIRE(RNG::getSeed() == seed);
}

TEST_CASE("GlobalTerrain_NoAvailableTerrains_ThrowsGeneratorError", "[global-terrain][selection]")
{
    // Arrange
    RulesFixture fixture;
    auto &deployment = fixture.createDeployment("EMPTY", "useGlobalTerrain: true");
    MissionSite site(nullptr, &deployment, nullptr);

    // Act
    const auto select = [&] { GlobalTerrain::selectTerrain(fixture.getMod(), deployment, nullptr, nullptr, &site); };

    // Assert
    REQUIRE_THROWS_WITH(select(), "Map generator encountered an error: No valid terrain found.");
}

TEST_CASE("GlobalTerrain_UnknownTerrainReference_PreservesExistingLookupError", "[global-terrain][selection]")
{
    // Arrange
    GlobalTerrainFixture fixture;
    loadTexture(fixture.globeTexture, fixture.rules.getMod(), "terrain: [{name: UNKNOWN}]");

    // Act
    const auto select = [&] {
        GlobalTerrain::selectTerrain(fixture.rules.getMod(), fixture.deployment, nullptr, &fixture.globeTexture,
                                     &fixture.site);
    };

    // Assert
    REQUIRE_THROWS_WITH(select(), "Terrain UNKNOWN not found");
}

TEST_CASE("GlobalTerrain_NoMatchingGeographicCriteria_ThrowsWithoutUsingStoredSiteTexture",
          "[global-terrain][selection]")
{
    // Arrange
    GlobalTerrainFixture fixture;
    loadTexture(fixture.globeTexture, fixture.rules.getMod(), "terrain: [{name: FOREST, area: [180, 360, -90, 90]}]");

    // Act
    const auto select = [&] {
        GlobalTerrain::selectTerrain(fixture.rules.getMod(), fixture.deployment, nullptr, &fixture.globeTexture,
                                     &fixture.site);
    };

    // Assert
    REQUIRE_THROWS_WITH(select(), "Map generator encountered an error: No valid terrain found.");
}

TEST_CASE("GlobalTerrain_NoMissionSite_PreservesTerrainWithoutConsumingRandomness", "[global-terrain][selection]")
{
    // Arrange
    GlobalTerrainFixture fixture;
    const bool preselected = GENERATE(false, true);
    const bool missingTexture = GENERATE(false, true);
    CAPTURE(preselected, missingTexture);
    auto *terrain = preselected ? &fixture.urban : nullptr;
    loadTexture(fixture.globeTexture, fixture.rules.getMod(), "terrain: [{name: UNKNOWN}]");
    const auto seed = RNG::getSeed();

    // Act
    const auto *selected = GlobalTerrain::selectTerrain(fixture.rules.getMod(), fixture.deployment, terrain,
                                                        missingTexture ? nullptr : &fixture.globeTexture, nullptr);

    // Assert
    REQUIRE(selected == terrain);
    REQUIRE(RNG::getSeed() == seed);
}
