#include "Fixtures.h"
#include "OppositeForce/Geoscape/FundingNames.h"
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

static void prepareFundingReport(RulesFixture &fixture, Language &language)
{
    fixture.createCountry("A", "fundingName: ACTOR");
    fixture.createCountry("B");
    fixture.createCountry("C");
    loadLanguage(language,
                 "strings:\n"
                 "  ACTOR: Foundation\n"
                 "  B: Beta\n"
                 "  C: Gamma\n"
                 "  STR_COUNTRIES_COMMA: '{0}, {1}'\n"
                 "  STR_COUNTRIES_AND: '{0} and {1}'\n"
                 "  SINGULAR: '{0} supports us.'\n"
                 "  PLURAL: '{0} support us.'\n");
}

TEST_CASE("FundingNames_ListWithOverride_FormatsTranslatedSingularPluralAndSeparators", "[funding][reports]")
{
    // Arrange
    const auto [countries, expected] = GENERATE(table<std::vector<std::string>, std::string>({
        {{"A"}, "\n\nFoundation supports us."},
        {{"A", "B"}, "\n\nFoundation and Beta support us."},
        {{"A", "B", "C"}, "\n\nFoundation, Beta and Gamma support us."},
        {{"B", "A"}, "\n\nBeta and Foundation support us."},
        {{"B", "C", "A"}, "\n\nBeta, Gamma and Foundation support us."},
        {{"A", "UNKNOWN"}, "\n\nFoundation and UNKNOWN support us."},
    }));
    CAPTURE(countries);
    RulesFixture fixture;
    Language language;
    prepareFundingReport(fixture, language);
    std::string text = "existing";

    // Act
    const bool formatted =
        FundingNames::tryFormatCountryList(fixture.getMod(), language, countries, "SINGULAR", "PLURAL", text);

    // Assert
    REQUIRE(formatted);
    REQUIRE(text == expected);
}

TEST_CASE("FundingNames_ListWithoutOverride_LeavesExistingTextUnchanged", "[funding][reports]")
{
    // Arrange
    const auto countries = GENERATE(std::vector<std::string>{}, std::vector<std::string>{"B"},
                                    std::vector<std::string>{"B", "C"}, std::vector<std::string>{"UNKNOWN"});
    RulesFixture fixture;
    Language language;
    prepareFundingReport(fixture, language);
    std::string text = "original report";

    // Act
    const bool formatted =
        FundingNames::tryFormatCountryList(fixture.getMod(), language, countries, "SINGULAR", "PLURAL", text);

    // Assert
    REQUIRE_FALSE(formatted);
    REQUIRE(text == "original report");
}
