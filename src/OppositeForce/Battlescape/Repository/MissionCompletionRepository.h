#pragma once

#include "../../../Engine/Yaml.h"

namespace OpenXcom
{
    class MissionCompletion;
}  // namespace OpenXcom

namespace OpenXcom::MissionCompletionRepository
{
    /// Restores completion state, using native defaults for missing save fields.
    void load(MissionCompletion &completion, const YAML::YamlNodeReader &reader);

    /// Writes completion state into the existing battle save mapping.
    void save(const MissionCompletion &completion, YAML::YamlNodeWriter writer);
}  // namespace OpenXcom::MissionCompletionRepository
