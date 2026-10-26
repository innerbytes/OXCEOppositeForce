#include "Fixtures.h"
#include "FundingFixtures.h"
#include "OppositeForce/Geoscape/Game/FundingReports.h"
#include "OppositeForce/Geoscape/Rules/FundingNames.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OppositeForce;
using namespace OppositeForce::Tests;

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
        FundingReports::tryFormatCountryList(fixture.getMod(), language, countries, "SINGULAR", "PLURAL", text);

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
        FundingReports::tryFormatCountryList(fixture.getMod(), language, countries, "SINGULAR", "PLURAL", text);

    // Assert
    REQUIRE_FALSE(formatted);
    REQUIRE(text == "original report");
}
