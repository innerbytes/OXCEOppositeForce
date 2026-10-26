#include <algorithm>

#include "Engine/Exception.h"
#include "Fixtures.h"
#include "Mod/MapScript.h"
#include "Mod/Texture.h"
#include "OppositeForce/Battlescape/Game/GlobalTerrain.h"
#include "Savegame/MissionSite.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

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
