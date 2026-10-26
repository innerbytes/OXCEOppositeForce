#pragma once

#include "Fixtures.h"

namespace OpenXcom::OppositeForceTests
{
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
}  // namespace OpenXcom::OppositeForceTests
