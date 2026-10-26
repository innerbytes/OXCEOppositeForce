#pragma once

#include "../../../Engine/Yaml.h"

namespace OpenXcom
{
    class Node;
}  // namespace OpenXcom

namespace OppositeForce::TerrainSpawnNodeRepository
{
    /// Restores custom spawn size, defaulting older nodes to ordinary behavior.
    void load(OpenXcom::Node &node, const OpenXcom::YAML::YamlNodeReader &reader);

    /// Appends nonzero custom metadata without changing native node fields.
    void save(const OpenXcom::Node &node, OpenXcom::YAML::YamlNodeWriter writer);
}  // namespace OppositeForce::TerrainSpawnNodeRepository
