#include "Fixtures.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OppositeForce;
using namespace OppositeForce::Tests;

TEST_CASE("RuleSoldier_DefaultConstruction_AllowsAvatarSelection", "[avatar][rules]")
{
    // Arrange
    RuleSoldier rule("DEFAULT", 0);

    // Act
    const bool disabled = rule.ofGetDisableAvatarSelection();

    // Assert
    REQUIRE_FALSE(disabled);
}

TEST_CASE("RuleSoldier_OptionalAvatarRestriction_LoadsExpectedValue", "[avatar][rules]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, bool>({
        {"{}", false},
        {"disableAvatarSelection: false", false},
        {"disableAvatarSelection: true", true},
        {"refNode:\n  disableAvatarSelection: true", true},
        {"refNode:\n  disableAvatarSelection: true\ndisableAvatarSelection: false", false},
        {"refNode:\n  disableAvatarSelection: false\ndisableAvatarSelection: true", true},
    }));
    CAPTURE(yaml);
    RulesFixture fixture;
    RuleSoldier rule("OPTIONAL", 0);

    // Act
    fixture.loadRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofGetDisableAvatarSelection() == expected);
}

TEST_CASE("RuleSoldier_LayeredAvatarRestriction_PreservesOrOverridesPreviousValue", "[avatar][rules]")
{
    // Arrange
    const auto [initial, layer, expected] = GENERATE(table<const char *, const char *, bool>({
        {"disableAvatarSelection: true", "costBuy: 100", true},
        {"disableAvatarSelection: false", "costBuy: 100", false},
        {"disableAvatarSelection: true", "disableAvatarSelection: false", false},
        {"disableAvatarSelection: false", "disableAvatarSelection: true", true},
    }));
    RulesFixture fixture;
    auto &rule = fixture.createSoldier("LAYERED", initial);

    // Act
    fixture.loadRule(rule, layer);

    // Assert
    REQUIRE(rule.ofGetDisableAvatarSelection() == expected);
}
