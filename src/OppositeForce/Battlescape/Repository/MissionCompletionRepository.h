#pragma once

#include "../../../Engine/Yaml.h"

namespace OppositeForce
{
    class MissionCompletion;
}  // namespace OppositeForce

namespace OppositeForce::MissionCompletionRepository
{
    /// Restores completion state, using native defaults for missing save fields.
    void load(MissionCompletion &completion, const OpenXcom::YAML::YamlNodeReader &reader);

    /// Writes completion state into the existing battle save mapping.
    void save(const MissionCompletion &completion, OpenXcom::YAML::YamlNodeWriter writer);
}  // namespace OppositeForce::MissionCompletionRepository
