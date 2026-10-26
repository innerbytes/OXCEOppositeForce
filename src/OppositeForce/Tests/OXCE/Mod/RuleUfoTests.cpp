#include "Fixtures.h"
#include "Savegame/MissionSite.h"
#include "Savegame/Waypoint.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OppositeForce;
using namespace OppositeForce::Tests;

TEST_CASE("RuleUfo_DefaultConstruction_IsVisible", "[hidden-ufo][rules]")
{
    // Arrange
    RuleUfo rule("DEFAULT");

    // Act
    const bool hidden = rule.ofIsHidden();

    // Assert
    REQUIRE_FALSE(hidden);
}

TEST_CASE("RuleUfo_OptionalHiddenValues_LoadsExpectedVisibility", "[hidden-ufo][rules]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, bool>({
        {"{}", false},
        {"isHidden: false", false},
        {"isHidden: true", true},
        {"refNode:\n  isHidden: true", true},
        {"refNode:\n  isHidden: true\nisHidden: false", false},
        {"refNode:\n  isHidden: false\nisHidden: true", true},
    }));
    CAPTURE(yaml);
    RulesFixture fixture;
    RuleUfo rule("OPTIONAL");

    // Act
    fixture.loadRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofIsHidden() == expected);
}

TEST_CASE("RuleUfo_LayeredHiddenValues_PreservesOrOverridesPreviousValue", "[hidden-ufo][rules]")
{
    // Arrange
    const auto [initial, layer, expected] = GENERATE(table<const char *, const char *, bool>({
        {"isHidden: true", "size: STR_SMALL", true},
        {"isHidden: false", "size: STR_SMALL", false},
        {"isHidden: true", "isHidden: false", false},
        {"isHidden: false", "isHidden: true", true},
    }));
    CAPTURE(initial, layer);
    RulesFixture fixture;
    auto &rule = fixture.createUfo("LAYERED", initial);

    // Act
    fixture.loadRule(rule, layer);

    // Assert
    REQUIRE(rule.ofIsHidden() == expected);
}
