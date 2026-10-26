#include "FundingReports.h"

#include "../../../Engine/Language.h"
#include "../../../Engine/LocalizedText.h"
#include "../Rules/FundingNames.h"

namespace OpenXcom::FundingReports
{
    bool tryFormatCountryList(const Mod &mod, const Language &language, const std::vector<std::string> &countries,
                              const std::string &singular, const std::string &plural, std::string &text)
    {
        bool hasOverride = false;
        for (const auto &countryId : countries)
        {
            if (FundingNames::getNameKey(mod, countryId) != countryId)
            {
                hasOverride = true;
                break;
            }
        }
        if (!hasOverride) return false;

        LocalizedText list = language.getString(FundingNames::getNameKey(mod, countries.front()));
        for (size_t index = 1; index < countries.size(); ++index)
        {
            const char *separator = index + 1 == countries.size() ? "STR_COUNTRIES_AND" : "STR_COUNTRIES_COMMA";
            list = language.getString(separator).arg(list).arg(
                language.getString(FundingNames::getNameKey(mod, countries[index])));
        }
        text = "\n\n" + std::string(language.getString(countries.size() == 1 ? singular : plural).arg(list));
        return true;
    }
}  // namespace OpenXcom::FundingReports
