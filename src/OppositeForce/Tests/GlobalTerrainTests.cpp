#include <algorithm>

#include "Engine/Exception.h"
#include "Fixtures.h"
#include "Mod/MapScript.h"
#include "Mod/Texture.h"
#include "OppositeForce/Battlescape/GlobalTerrain.h"
#include "Savegame/MissionSite.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

namespace
{
    void loadTexture(Texture &texture, Mod &mod, const std::string &yaml)
    {
        YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "test texture");
        texture.load(reader, &mod);
    }

    class GlobalTerrainFixture
    {
    public:
        RulesFixture rules;
        AlienDeployment &deployment;
        RuleTerrain &urban;
        RuleTerrain &forest;
        Texture siteTexture{1};
        Texture globeTexture{2};
        MissionSite site;

        explicit GlobalTerrainFixture(const std::string &yaml = "useGlobalTerrain: true")
            : deployment(rules.createDeployment("RESEARCH", yaml)),
              urban(rules.createTerrain("URBAN", "script: URBAN_SCRIPT")),
              forest(rules.createTerrain("FOREST",
                                         "script: FOREST_SCRIPT\nmapBlocks:\n"
                                         "  - {name: FOREST_A, width: 10, length: 10, groups: [0, 1]}\n"
                                         "  - {name: FOREST_B, width: 10, length: 10, groups: [0, 1]}\n")),
              site(nullptr, &deployment, nullptr)
        {
            loadTexture(siteTexture, rules.getMod(), "terrain: [{name: URBAN}]\nstartingCondition: SITE_CONDITION\n");
            loadTexture(globeTexture, rules.getMod(),
                        "terrain: [{name: FOREST}]\nstartingCondition: GLOBE_CONDITION\n");
            site.setTexture(1);
            site.setLongitude(Deg2Rad(20));
            site.setLatitude(Deg2Rad(10));
        }
    };

    std::vector<MapBlock *> selectBlocks(MapScript &command, RuleTerrain &terrain)
    {
        std::vector<MapBlock *> blocks;
        command.init();
        for (int attempt = 0; attempt < 6; ++attempt) blocks.push_back(command.getNextBlock(&terrain));
        return blocks;
    }
}  // namespace

TEST_CASE("AlienDeployment_DefaultConstruction_DisablesGlobalTerrain", "[global-terrain][rules]")
{
    // Arrange
    AlienDeployment deployment("DEFAULT");

    // Act
    const bool enabled = deployment.ofGetUseGlobalTerrain();

    // Assert
    REQUIRE_FALSE(enabled);
}

TEST_CASE("AlienDeployment_OptionalGlobalTerrain_LoadsEffectiveBoolean", "[global-terrain][rules]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, bool>({
        {"{}", false},
        {"useGlobalTerrain: false", false},
        {"useGlobalTerrain: true", true},
        {"refNode:\n  useGlobalTerrain: true", true},
        {"refNode:\n  useGlobalTerrain: true\nuseGlobalTerrain: false", false},
        {"refNode:\n  useGlobalTerrain: false\nuseGlobalTerrain: true", true},
    }));
    CAPTURE(yaml);
    RulesFixture fixture;
    AlienDeployment deployment("OPTIONAL");

    // Act
    fixture.loadRule(deployment, yaml);

    // Assert
    REQUIRE(deployment.ofGetUseGlobalTerrain() == expected);
}

TEST_CASE("AlienDeployment_LayeredGlobalTerrain_PreservesOrOverridesPreviousValue", "[global-terrain][rules]")
{
    // Arrange
    const auto [initial, layer, expected] = GENERATE(table<const char *, const char *, bool>({
        {"useGlobalTerrain: true", "width: 40", true},
        {"useGlobalTerrain: false", "width: 40", false},
        {"useGlobalTerrain: true", "useGlobalTerrain: false", false},
        {"useGlobalTerrain: false", "useGlobalTerrain: true", true},
    }));
    CAPTURE(initial, layer);
    RulesFixture fixture;
    auto &deployment = fixture.createDeployment("LAYERED", initial);

    // Act
    fixture.loadRule(deployment, layer);

    // Assert
    REQUIRE(deployment.ofGetUseGlobalTerrain() == expected);
}

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
    if (longitude >= 60 || latitude < 0) REQUIRE(selected->getName() == "DESERT");
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

    // Act
    const auto select = [&] { GlobalTerrain::selectTerrain(fixture.getMod(), deployment, nullptr, nullptr, nullptr); };

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

TEST_CASE("GlobalTerrain_MissingMissionTarget_ThrowsInsteadOfDereferencingNull", "[global-terrain][selection]")
{
    // Arrange
    GlobalTerrainFixture fixture;

    // Act
    const auto select = [&] {
        GlobalTerrain::selectTerrain(fixture.rules.getMod(), fixture.deployment, nullptr, &fixture.globeTexture,
                                     nullptr);
    };

    // Assert
    REQUIRE_THROWS_WITH(select(), Catch::Matchers::ContainsSubstring("No mission target") &&
                                      Catch::Matchers::ContainsSubstring("RESEARCH"));
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
