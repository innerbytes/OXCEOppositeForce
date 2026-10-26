#include "Fixtures.h"
#include "OppositeForce/Geoscape/Rules/HangarRules.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

TEST_CASE("RuleBaseFacility_OptionalHangarType_LoadsEffectiveTypeAndCapacity", "[hangars][rules]")
{
    // Arrange
    const auto [yaml, expectedType, capacity] = GENERATE(table<const char *, const char *, int>({
        {"crafts: 2", "", 2},
        {"crafts: 1\nhangarType: A", "A", 1},
        {"refNode:\n  crafts: 1\n  hangarType: A", "A", 1},
        {"refNode:\n  crafts: 1\n  hangarType: A\nhangarType: B", "B", 1},
    }));
    CAPTURE(yaml);
    RulesFixture fixture;
    RuleBaseFacility rule("HANGAR", 0);

    // Act
    fixture.loadRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofGetHangarCompatibilityRule().hangarType == expectedType);
    REQUIRE(rule.getCrafts() == capacity);
}

TEST_CASE("RuleBaseFacility_LayeredHangarType_PreservesOrOverridesPreviousType", "[hangars][rules]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, const char *>({
        {"radarRange: 100", "A"},
        {"hangarType: B", "B"},
    }));
    RulesFixture fixture;
    auto &rule = fixture.createFacility("HANGAR", "crafts: 1\nhangarType: A");

    // Act
    fixture.loadRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofGetHangarCompatibilityRule().hangarType == expected);
}

TEST_CASE("RuleBaseFacility_EmptyHangarType_ThrowsValidationException", "[hangars][rules]")
{
    // Arrange
    RulesFixture fixture;
    RuleBaseFacility rule("EMPTY", 0);

    // Act
    const auto load = [&] { fixture.loadRule(rule, "crafts: 1\nhangarType: ''"); };

    // Assert
    REQUIRE_THROWS_AS(load(), Exception);
}

TEST_CASE("RuleBaseFacility_TypedFacilityWithInvalidEffectiveCapacity_ThrowsDuringAfterLoad", "[hangars][rules]")
{
    // Arrange
    const auto yaml = GENERATE("crafts: 0\nhangarType: A", "crafts: 2\nhangarType: A",
                               "refNode:\n  crafts: 1\n  hangarType: A\ncrafts: 3");
    RulesFixture fixture;
    auto &rule = fixture.createFacility("INVALID", yaml);

    // Act
    const auto validate = [&] { rule.afterLoad(&fixture.getMod()); };

    // Assert
    REQUIRE_THROWS_AS(validate(), Exception);
}
