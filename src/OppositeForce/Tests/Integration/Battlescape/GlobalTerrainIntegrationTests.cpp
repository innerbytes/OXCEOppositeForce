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
using namespace OppositeForce;
using namespace OppositeForce::Tests;

TEST_CASE("GlobalTerrain_WeightedGeographicCriteria_MatchesLandedUfoSelectionAndRandomness",
          "[global-terrain][selection]")
{
    // Arrange
    GlobalTerrainFixture fixture;
    const int seed = GENERATE(range(1, 9));
    const auto [longitude, latitude] = GENERATE(table<int, int>({{20, 10}, {20, -10}, {80, 10}, {80, -10}}));
    CAPTURE(seed, longitude, latitude);
    fixture.rules.createTerrain("DESERT", "{}");
    loadTexture(fixture.globeTexture, fixture.rules.getMod(),
                "terrain:\n"
                "  - {name: FOREST, weight: 1, area: [0, 60, 0, 90]}\n"
                "  - {name: DESERT, weight: 9, area: [0, 360, -90, 90]}\n"
                "  - {name: UNKNOWN, weight: 100, area: [180, 360, -90, 90]}\n");
    fixture.site.setLongitude(Deg2Rad(longitude));
    fixture.site.setLatitude(Deg2Rad(latitude));
    auto &ufoRule = fixture.rules.createUfo("LANDED");
    Ufo landed(&ufoRule, 1);
    landed.setLongitude(fixture.site.getLongitude());
    landed.setLatitude(fixture.site.getLatitude());
    RNG::setSeed(seed);
    const auto expected = fixture.globeTexture.getRandomTerrain(&landed);
    const auto expectedSeed = RNG::getSeed();
    RNG::setSeed(seed);

    // Act
    const auto *selected = GlobalTerrain::selectTerrain(fixture.rules.getMod(), fixture.deployment, nullptr,
                                                        &fixture.globeTexture, &fixture.site);

    // Assert
    REQUIRE(selected != nullptr);
    REQUIRE(selected->getName() == expected);
    REQUIRE(RNG::getSeed() == expectedSeed);
    if (longitude >= 60 || latitude < 0)
    {
        REQUIRE(selected->getName() == "DESERT");
    }
}

TEST_CASE("GlobalTerrain_DeploymentScriptOverrides_RetainsScriptAndMapScriptsPrecedence", "[global-terrain][scripts]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, const char *>({
        {"useGlobalTerrain: true", ""},
        {"useGlobalTerrain: true\nscript: CUSTOM_SCRIPT", "CUSTOM_SCRIPT"},
        {"useGlobalTerrain: true\nscript: CUSTOM_SCRIPT\nmapScripts: [LIST_SCRIPT]", "LIST_SCRIPT"},
        {"useGlobalTerrain: true\nscript: CUSTOM_SCRIPT\nmapScripts: []", "CUSTOM_SCRIPT"},
    }));
    CAPTURE(yaml);
    GlobalTerrainFixture fixture(yaml);

    // Act
    const auto *selected = GlobalTerrain::selectTerrain(fixture.rules.getMod(), fixture.deployment, nullptr,
                                                        &fixture.globeTexture, &fixture.site);
    const auto &terrainScript = selected->getRandomMapScript();
    const auto &deploymentScript = fixture.deployment.getRandomMapScript();

    // Assert
    REQUIRE(selected == &fixture.forest);
    REQUIRE(terrainScript == "FOREST_SCRIPT");
    REQUIRE(deploymentScript == expected);
}

TEST_CASE("GlobalTerrain_NormalTerrainScript_PreservesBlockFrequenciesPlacementAndCraftCommands",
          "[global-terrain][scripts]")
{
    // Arrange
    GlobalTerrainFixture fixture("useGlobalTerrain: true\nwidth: 40\nlength: 50\nheight: 4\ncustomUfo: ENEMY_CRAFT");
    const auto &commands =
        fixture.rules.createMapScript("FOREST_SCRIPT",
                                      "- {type: addCraft, rects: [[0, 0, 2, 2]]}\n"
                                      "- {type: addUFO, rects: [[2, 2, 2, 2]]}\n"
                                      "- {type: fillArea, blocks: [0, 1], freqs: [1, 3], maxUses: [1, 2]}\n");
    const auto seed = RNG::getSeed();
    auto *nativeTerrain = fixture.rules.getMod().getTerrain(fixture.globeTexture.getRandomTerrain(&fixture.site), true);
    const auto *nativeScript = fixture.rules.getMod().getMapScript(nativeTerrain->getRandomMapScript());
    const auto expectedBlocks = selectBlocks(*nativeScript->at(2), *nativeTerrain);
    const auto expectedSeed = RNG::getSeed();
    RNG::setSeed(seed);
    int width = 0, length = 0, height = 0;

    // Act
    auto *selected = GlobalTerrain::selectTerrain(fixture.rules.getMod(), fixture.deployment, nullptr,
                                                  &fixture.globeTexture, &fixture.site);
    const auto *selectedScript = fixture.rules.getMod().getMapScript(selected->getRandomMapScript());
    const auto blocks = selectBlocks(*selectedScript->at(2), *selected);
    fixture.deployment.getDimensions(&width, &length, &height);

    // Assert
    REQUIRE(selectedScript == &commands);
    REQUIRE(blocks == expectedBlocks);
    REQUIRE(RNG::getSeed() == expectedSeed);
    REQUIRE(std::count(blocks.begin(), blocks.end(), fixture.forest.getMapBlocks()->at(0)) == 1);
    REQUIRE(std::count(blocks.begin(), blocks.end(), fixture.forest.getMapBlocks()->at(1)) == 2);
    REQUIRE(std::count(blocks.begin(), blocks.end(), nullptr) == 3);
    REQUIRE(commands.at(0)->getType() == MSC_ADDCRAFT);
    REQUIRE(commands.at(0)->getGroups()->front() == 1);
    REQUIRE(commands.at(0)->getRects()->front()->w == 2);
    REQUIRE(commands.at(1)->getType() == MSC_ADDUFO);
    REQUIRE(commands.at(1)->getRects()->front()->x == 2);
    REQUIRE(commands.at(2)->getType() == MSC_FILLAREA);
    REQUIRE(fixture.deployment.getCustomUfoName() == "ENEMY_CRAFT");
    REQUIRE(width == 40);
    REQUIRE(length == 50);
    REQUIRE(height == 4);
}
