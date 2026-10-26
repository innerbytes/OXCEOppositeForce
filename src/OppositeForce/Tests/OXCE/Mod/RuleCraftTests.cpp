#include "Fixtures.h"
#include "OppositeForce/Geoscape/Rules/HangarRules.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

TEST_CASE("RuleCraft_OptionalAllowedHangarTypes_LoadsEffectiveRestrictions", "[hangars][rules]")
{
    // Arrange
    const auto [yaml, specified, expected] = GENERATE(table<const char *, bool, std::vector<std::string>>({
        {"{}", false, {}},
        {"allowedHangarTypes: [A, B]", true, {"A", "B"}},
        {"allowedHangarTypes: [A, B, A]", true, {"A", "B"}},
        {"refNode:\n  allowedHangarTypes: [A, B]", true, {"A", "B"}},
        {"refNode:\n  allowedHangarTypes: [A, B]\nallowedHangarTypes: [C]", true, {"C"}},
    }));
    CAPTURE(yaml);
    RulesFixture fixture;
    RuleCraft rule("CRAFT", 0);

    // Act
    fixture.loadRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofGetHangarCompatibilityRule().allowedHangarTypesSpecified == specified);
    REQUIRE(rule.ofGetHangarCompatibilityRule().allowedHangarTypes == expected);
}

TEST_CASE("RuleCraft_LayeredAllowedHangarTypes_PreservesOrReplacesPreviousList", "[hangars][rules]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, std::vector<std::string>>({
        {"costBuy: 100", {"A", "B"}},
        {"allowedHangarTypes: [C, C]", {"C"}},
    }));
    RulesFixture fixture;
    auto &rule = fixture.createCraft("CRAFT", "allowedHangarTypes: [A, B]");

    // Act
    fixture.loadRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofGetHangarCompatibilityRule().allowedHangarTypesSpecified);
    REQUIRE(rule.ofGetHangarCompatibilityRule().allowedHangarTypes == expected);
}

TEST_CASE("RuleCraft_InvalidAllowedHangarTypes_ThrowsValidationException", "[hangars][rules]")
{
    // Arrange
    const auto yaml =
        GENERATE("allowedHangarTypes: A", "allowedHangarTypes: []", "allowedHangarTypes: ['']",
                 "allowedHangarTypes: [A, '']", "refNode:\n  allowedHangarTypes: [A]\nallowedHangarTypes: []");
    CAPTURE(yaml);
    RulesFixture fixture;
    RuleCraft rule("INVALID", 0);

    // Act
    const auto load = [&] { fixture.loadRule(rule, yaml); };

    // Assert
    REQUIRE_THROWS_AS(load(), Exception);
}
