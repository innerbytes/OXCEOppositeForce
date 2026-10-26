#pragma once

#include <string>
#include <vector>

namespace OpenXcom
{
    class Language;

    class Mod;
}  // namespace OpenXcom

namespace OppositeForce::FundingReports
{
    /// Formats a monthly report sentence only when at least one name is overridden.
    bool tryFormatCountryList(const OpenXcom::Mod &mod, const OpenXcom::Language &language,
                              const std::vector<std::string> &countries, const std::string &singular,
                              const std::string &plural, std::string &text);
}  // namespace OppositeForce::FundingReports
