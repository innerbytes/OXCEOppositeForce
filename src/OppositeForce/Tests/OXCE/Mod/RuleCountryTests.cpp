#include "Fixtures.h"
#include "OppositeForce/Geoscape/Rules/FundingNames.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OppositeForce;
using namespace OppositeForce::Tests;

TEST_CASE("RuleCountry_OptionalFundingName_LoadsExpectedKey", "[funding][rules]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, const char *>({
        {"{}", "STR_COUNTRY"},
        {"fundingName: STR_ACTOR", "STR_ACTOR"},
        {"fundingName: ''", "STR_COUNTRY"},
        {"refNode:\n  fundingName: STR_PARENT", "STR_PARENT"},
        {"refNode:\n  fundingName: STR_PARENT\nfundingName: STR_CHILD", "STR_CHILD"},
        {"refNode:\n  fundingName: STR_PARENT\nfundingName: ''", "STR_COUNTRY"},
    }));
    CAPTURE(yaml);
    RulesFixture fixture;
    RuleCountry country("STR_COUNTRY");

    // Act
    fixture.loadRule(country, yaml);

    // Assert
    REQUIRE(FundingNames::getNameKey(country) == expected);
    REQUIRE(country.getType() == "STR_COUNTRY");
}

TEST_CASE("RuleCountry_LayeredFundingName_PreservesOrOverridesPreviousKey", "[funding][rules]")
{
    // Arrange
    const auto [layer, expected] = GENERATE(table<const char *, const char *>({
        {"fundingCap: 100", "STR_ACTOR"},
        {"fundingName: STR_UPDATED", "STR_UPDATED"},
        {"fundingName: ''", "STR_COUNTRY"},
    }));
    RulesFixture fixture;
    auto &country = fixture.createCountry("STR_COUNTRY", "fundingName: STR_ACTOR");

    // Act
    fixture.loadRule(country, layer);

    // Assert
    REQUIRE(FundingNames::getNameKey(country) == expected);
}

TEST_CASE("RuleCountry_FundingNameOverride_PreservesGeographicIdentityAndFunding", "[funding]")
{
    // Arrange
    RulesFixture fixture;
    auto &country = fixture.createCountry("STR_COUNTRY", "areas: [[0, 90, -45, 45]]\nfundingCap: 250");
    const auto longitude = country.getLonMax();
    const auto latitude = country.getLatMin();

    // Act
    fixture.loadRule(country, "fundingName: STR_ACTOR");

    // Assert
    REQUIRE(country.getType() == "STR_COUNTRY");
    REQUIRE(country.getLonMax() == longitude);
    REQUIRE(country.getLatMin() == latitude);
    REQUIRE(country.getFundingCap() == 250);
}
