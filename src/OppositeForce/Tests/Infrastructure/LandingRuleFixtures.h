#pragma once

#include "Fixtures.h"

namespace OppositeForce::Tests
{
    using namespace OpenXcom;

    static void loadMissionRule(RuleAlienMission &rule, const std::string &yaml)
    {
        YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "landing mission rule");
        rule.load(reader);
    }
}  // namespace OppositeForce::Tests
