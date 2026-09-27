/*
 * OXCE-OF: Parse and validate the optional hangar compatibility properties.
 * The rules are deliberately independent of OXCE's normal craft capacity rules.
 */

#include "HangarRules.h"

#include <algorithm>

#include "../../Engine/Exception.h"

namespace OpenXcom
{

    namespace HangarRules
    {

        void loadFacility(Facility &rule, const YAML::YamlNodeReader &reader, const std::string &name)
        {
            if (reader)
            {
                rule.hangarType = reader.readVal<std::string>();
                if (rule.hangarType.empty())
                {
                    throw Exception("Hangar type in " + name + " must not be empty.");
                }
            }
        }

        void validateFacility(const Facility &rule, const std::string &name, int crafts)
        {
            if (!rule.hangarType.empty() && crafts != 1)
            {
                throw Exception("Typed hangar " + name + " must have crafts: 1.");
            }
        }

        void loadCraft(Craft &rule, const YAML::YamlNodeReader &reader, const std::string &name)
        {
            if (!reader) return;
            if (!reader.isSeq())
            {
                throw Exception("Craft " + name + " must list allowed hangar types.");
            }
            rule.allowedHangarTypesSpecified = true;
            rule.allowedHangarTypes.clear();
            for (const auto &type : reader.children())
            {
                std::string value = type.readVal<std::string>();
                if (value.empty())
                {
                    throw Exception("Craft " + name + " has an empty allowed hangar type.");
                }
                if (std::find(rule.allowedHangarTypes.begin(), rule.allowedHangarTypes.end(), value) ==
                    rule.allowedHangarTypes.end())
                {
                    rule.allowedHangarTypes.push_back(value);
                }
            }
            if (rule.allowedHangarTypes.empty())
            {
                throw Exception("Craft " + name + " must allow at least one hangar type.");
            }
        }

        bool accepts(const Facility &facility, const Craft &craft)
        {
            return facility.hangarType.empty() || !craft.allowedHangarTypesSpecified ||
                   std::find(craft.allowedHangarTypes.begin(), craft.allowedHangarTypes.end(), facility.hangarType) !=
                       craft.allowedHangarTypes.end();
        }

    }  // namespace HangarRules

}  // namespace OpenXcom
