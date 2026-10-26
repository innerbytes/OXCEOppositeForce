#include <filesystem>
#include <functional>
#include <memory>

#include "Engine/FileMap.h"
#include "Engine/Game.h"
#include "Engine/Options.h"
#include "Engine/SurfaceSet.h"
#include "Fixtures.h"
#include "Geoscape/Globe.h"
#include "LandingRuleFixtures.h"
#include "Mod/AlienRace.h"
#include "Mod/RuleGlobe.h"
#include "Mod/Texture.h"
#include "Mod/UfoTrajectory.h"
#include "OppositeForce/Battlescape/Game/GlobalTerrain.h"
#include "OppositeForce/Geoscape/Game/LandingTextures.h"
#include "Savegame/MissionSite.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OppositeForce;
using namespace OppositeForce::Tests;

TEST_CASE("RuleAlienMission_ExcludedLandingTextures_LoadsDefaultsAndInheritance", "[landing-textures][rules]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, std::vector<int>>({
        {"{}", {}},
        {"excludedLandingTextures: [9, 12]", {9, 12}},
        {"refNode: {excludedLandingTextures: [9, 12]}", {9, 12}},
        {"refNode: {excludedLandingTextures: [9, 12]}\nexcludedLandingTextures: [3]", {3}},
        {"refNode: {excludedLandingTextures: [9, 12]}\nexcludedLandingTextures: []", {}},
    }));
    RuleAlienMission rule("RULE");

    // Act
    loadMissionRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofGetExcludedLandingTextures() == expected);
}

TEST_CASE("RuleAlienMission_ExcludedLandingTextureLayers_PreservesReplacesOrClears", "[landing-textures][rules]")
{
    // Arrange
    const auto [layer, expected] = GENERATE(table<const char *, std::vector<int>>({
        {"points: 1", {9, 12}},
        {"excludedLandingTextures: [2]", {2}},
        {"excludedLandingTextures: []", {}},
    }));
    RuleAlienMission rule("RULE");
    loadMissionRule(rule, "excludedLandingTextures: [9, 12]");

    // Act
    loadMissionRule(rule, layer);

    // Assert
    REQUIRE(rule.ofGetExcludedLandingTextures() == expected);
}
