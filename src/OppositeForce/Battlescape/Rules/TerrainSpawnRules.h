#pragma once

#include <map>
#include <string>
#include <vector>

#include "../../../Battlescape/Position.h"
#include "../../../Engine/Yaml.h"

namespace OppositeForce::TerrainSpawnRules
{
    struct Point
    {
        OpenXcom::Position position;

        int unitSize = 1;

        int rank = 0;

        int priority = 1;

        std::string source;
    };

    struct Block
    {
        std::vector<Point> points;

        std::string source;
    };

    using Rules = std::map<std::string, Block>;

    /// Loads one ruleset layer without changing the terrain's map block list.
    void loadRules(Rules &rules, const OpenXcom::YAML::YamlNodeReader &reader, const std::string &terrain);

    /// Formats an actionable diagnostic that remains valid after YAML readers expire.
    std::string describePoint(const std::string &terrain, const std::string &block, const Point &point);

    /// Rejects out-of-bounds or overlapping custom footprints against actual MAP dimensions.
    void validateFootprints(const std::string &terrain, const std::string &block, const Block &rule,
                            const OpenXcom::Position &dimensions);
}  // namespace OppositeForce::TerrainSpawnRules
