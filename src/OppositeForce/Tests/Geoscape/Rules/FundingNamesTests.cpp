#include "Fixtures.h"
#include "OppositeForce/Geoscape/Rules/FundingNames.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

TEST_CASE("FundingNames_CountryWithoutOverride_ReturnsCountryId", "[funding][rules]")
{
    // Arrange
    RulesFixture fixture;
    auto &country = fixture.createCountry("STR_COUNTRY");

    // Act
    const auto key = FundingNames::getNameKey(country);

    // Assert
    REQUIRE(key == "STR_COUNTRY");
}

TEST_CASE("FundingNames_ModLookup_ReturnsOverrideOrOriginalId", "[funding]")
{
    // Arrange
    const auto [id, expected] = GENERATE(table<const char *, const char *>({
        {"STR_COUNTRY_A", "STR_ACTOR"},
        {"STR_COUNTRY_B", "STR_COUNTRY_B"},
        {"UNKNOWN", "UNKNOWN"},
    }));
    RulesFixture fixture;
    fixture.createCountry("STR_COUNTRY_A", "fundingName: STR_ACTOR");
    fixture.createCountry("STR_COUNTRY_B");
    const std::string countryId = id;

    // Act
    const auto key = FundingNames::getNameKey(fixture.getMod(), countryId);

    // Assert
    REQUIRE(key == expected);
}
