/* OXCE-OF: Funding actor presentation without changing engine country IDs. */
#include "FundingNames.h"

#include "../../Engine/Language.h"
#include "../../Engine/LocalizedText.h"
#include "../../Mod/Mod.h"
#include "../../Mod/RuleCountry.h"

namespace OpenXcom
{
    namespace FundingNames
    {
        const std::string &getNameKey(const RuleCountry &country)
        {
            return country.ofGetFundingName().empty() ? country.getType() : country.ofGetFundingName();
        }

        const std::string &getNameKey(const Mod &mod, const std::string &countryId)
        {
            const auto *country = mod.getCountry(countryId, false);
            return country ? getNameKey(*country) : countryId;
        }

        bool tryFormatCountryList(const Mod &mod, const Language &language, const std::vector<std::string> &countries,
                                  const std::string &singular, const std::string &plural, std::string &text)
        {
            bool hasOverride = false;
            for (const auto &countryId : countries)
            {
                if (getNameKey(mod, countryId) != countryId)
                {
                    hasOverride = true;
                    break;
                }
            }
            if (!hasOverride) return false;

            LocalizedText list = language.getString(getNameKey(mod, countries.front()));
            for (size_t index = 1; index < countries.size(); ++index)
            {
                const char *separator = index + 1 == countries.size() ? "STR_COUNTRIES_AND" : "STR_COUNTRIES_COMMA";
                list =
                    language.getString(separator).arg(list).arg(language.getString(getNameKey(mod, countries[index])));
            }
            text = "\n\n" + std::string(language.getString(countries.size() == 1 ? singular : plural).arg(list));
            return true;
        }
    }  // namespace FundingNames
}  // namespace OpenXcom
