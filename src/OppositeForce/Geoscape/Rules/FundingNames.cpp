/* OXCE-OF: Funding actor presentation without changing engine country IDs. */
#include "FundingNames.h"

#include "../../../Mod/Mod.h"
#include "../../../Mod/RuleCountry.h"

namespace OppositeForce::FundingNames
{
    using namespace OpenXcom;

    const std::string &getNameKey(const RuleCountry &country)
    {
        return country.ofGetFundingName().empty() ? country.getType() : country.ofGetFundingName();
    }

    const std::string &getNameKey(const Mod &mod, const std::string &countryId)
    {
        const auto *country = mod.getCountry(countryId, false);
        return country ? getNameKey(*country) : countryId;
    }
}  // namespace OppositeForce::FundingNames
