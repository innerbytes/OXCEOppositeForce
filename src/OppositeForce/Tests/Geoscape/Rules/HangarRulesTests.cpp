#include "Fixtures.h"
#include "OppositeForce/Geoscape/Rules/HangarRules.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

TEST_CASE("HangarRules_TypedFacilityWithSingleCapacity_PassesValidation", "[hangars][rules]")
{
    // Arrange
    RulesFixture fixture;
    auto &rule = fixture.createFacility("VALID", "refNode:\n  crafts: 1\nhangarType: A");

    // Act
    const auto validate = [&] {
        HangarRules::validateFacility(rule.ofGetHangarCompatibilityRule(), rule.getType(), rule.getCrafts());
    };

    // Assert
    REQUIRE_NOTHROW(validate());
}

TEST_CASE("HangarRules_UntypedFacilityWithAnyCapacity_PassesCompatibilityValidation", "[hangars][rules]")
{
    // Arrange
    const int capacity = GENERATE(0, 1, 3);
    HangarRules::Facility rule;

    // Act
    const auto validate = [&] { HangarRules::validateFacility(rule, "UNTYPED", capacity); };

    // Assert
    REQUIRE_NOTHROW(validate());
}

TEST_CASE("HangarRules_CraftAndFacilityPair_AppliesExactMatchingOrUnrestrictedFallback", "[hangars][rules]")
{
    // Arrange
    const auto [facilityYaml, craftYaml, expected] = GENERATE(table<const char *, const char *, bool>({
        {"crafts: 1\nhangarType: A", "allowedHangarTypes: [A]", true},
        {"crafts: 1\nhangarType: A", "allowedHangarTypes: [B]", false},
        {"crafts: 1\nhangarType: A", "allowedHangarTypes: [a]", false},
        {"crafts: 1\nhangarType: ARBITRARY_TYPE", "allowedHangarTypes: [ARBITRARY_TYPE]", true},
        {"crafts: 1", "allowedHangarTypes: [A]", true},
        {"crafts: 1\nhangarType: A", "{}", true},
        {"crafts: 1", "{}", true},
    }));
    RulesFixture fixture;
    auto &facility = fixture.createFacility("HANGAR", facilityYaml);
    auto &craft = fixture.createCraft("CRAFT", craftYaml);

    // Act
    const bool accepts =
        HangarRules::accepts(facility.ofGetHangarCompatibilityRule(), craft.ofGetHangarCompatibilityRule());

    // Assert
    REQUIRE(accepts == expected);
}
