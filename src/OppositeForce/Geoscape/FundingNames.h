/* OXCE-OF: Localized funding actor names, separate from country geography. */
#pragma once

#include <string>
#include <vector>

namespace OpenXcom
{
    class Language;
    class Mod;
    class RuleCountry;

    namespace FundingNames
    {
        /// Gets the actor localization key, falling back to the geographic country ID.
        const std::string &getNameKey(const RuleCountry &country);
        /// Resolves a country ID; unknown IDs are preserved for normal localization.
        const std::string &getNameKey(const Mod &mod, const std::string &countryId);
        /// Formats a monthly report sentence only when at least one name is overridden.
        bool tryFormatCountryList(const Mod &mod, const Language &language, const std::vector<std::string> &countries,
                                  const std::string &singular, const std::string &plural, std::string &text);
    }  // namespace FundingNames
}  // namespace OpenXcom
