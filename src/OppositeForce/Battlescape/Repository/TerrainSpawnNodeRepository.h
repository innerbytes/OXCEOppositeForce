#pragma once

#include "../../../Engine/Yaml.h"

namespace OpenXcom
{
    class Node;

    namespace TerrainSpawnNodeRepository
    {
        /// Restores custom spawn size, defaulting older nodes to ordinary behavior.
        void load(Node &node, const YAML::YamlNodeReader &reader);

        /// Appends nonzero custom metadata without changing native node fields.
        void save(const Node &node, YAML::YamlNodeWriter writer);
    }  // namespace TerrainSpawnNodeRepository
}  // namespace OpenXcom
