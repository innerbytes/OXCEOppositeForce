/* OXCE-OF: Localized funding actor names, separate from country geography. */
#pragma once

#include <string>

namespace OpenXcom
{
    class Mod;

    class RuleCountry;

    namespace FundingNames
    {
        /// Gets the actor localization key, falling back to the geographic country ID.
        const std::string &getNameKey(const RuleCountry &country);

        /// Resolves a country ID; unknown IDs are preserved for normal localization.
        const std::string &getNameKey(const Mod &mod, const std::string &countryId);
    }  // namespace FundingNames
}  // namespace OpenXcom
