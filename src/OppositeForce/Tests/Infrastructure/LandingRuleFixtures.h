#pragma once

#include "Fixtures.h"

namespace OpenXcom::OppositeForceTests
{
    static void loadMissionRule(RuleAlienMission &rule, const std::string &yaml)
    {
        YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "landing mission rule");
        rule.load(reader);
    }
}  // namespace OpenXcom::OppositeForceTests
