#include "TerrainSpawnRules.h"

#include <sstream>

#include "../../Engine/Exception.h"

namespace OpenXcom::TerrainSpawnRules
{
    static std::string getSource(const YAML::YamlNodeReader &reader)
    {
        try
        {
            const auto location = reader.getLocationInFile();
            std::ostringstream source;
            source << std::string(location.name.str ? location.name.str : "", location.name.len) << ':' << location.line
                   << ':' << location.col;
            return source.str();
        }
        catch (const std::exception &)
        {
            // Generated YAML and in-memory rule readers need not carry source locations.
            return "<generated ruleset>";
        }
    }

    std::string describePoint(const std::string &terrain, const std::string &block, const Point &point)
    {
        std::ostringstream message;
        message << point.source << ": terrain " << terrain << ", fragment " << block << ", position ["
                << point.position.x << ", " << point.position.y << ", " << point.position.z << "], unitSize "
                << point.unitSize;
        return message.str();
    }

    void loadRules(Rules &rules, const YAML::YamlNodeReader &reader, const std::string &terrain)
    {
        if (!reader) return;
        if (!reader.isMap())
            throw Exception(getSource(reader) + ": " + terrain + ": mapBlockSpawnPoints must be a mapping");
        for (const auto &entry : reader.children())
        {
            const auto name = entry.readKey<std::string>();
            Block block;
            block.source = getSource(entry);
            if (name.empty() || !entry.isSeq())
                throw Exception(block.source + ": " + terrain +
                                ": each fragment must have a name and a spawn-point list");
            for (const auto &item : entry.children())
            {
                Point point;
                point.source = getSource(item);
                const auto position = item["position"];
                if (!item.isMap() || !position.isSeq() || position.childrenCount() != 3 || !item["unitSize"])
                    throw Exception(describePoint(terrain, name, point) +
                                    ": position [x, y, z] and unitSize are required");
                point.position =
                    Position(position[0].readVal<int>(), position[1].readVal<int>(), position[2].readVal<int>());
                point.unitSize = item["unitSize"].readVal<int>();
                point.rank = item["rank"].readVal(0);
                point.priority = item["priority"].readVal(1);
                if ((point.unitSize != 1 && point.unitSize != 2) || point.rank < 0 || point.rank > 8 ||
                    point.priority < 1 || point.priority > 255)
                    throw Exception(describePoint(terrain, name, point) +
                                    ": unitSize must be 1 or 2, rank 0-8, and priority 1-255");
                block.points.push_back(point);
            }
            rules[name] = std::move(block);
        }
    }

    void validateFootprints(const std::string &terrain, const std::string &block, const Block &rule,
                            const Position &dimensions)
    {
        for (size_t index = 0; index < rule.points.size(); ++index)
        {
            const auto &point = rule.points[index];
            const auto &position = point.position;
            if (position.x < 0 || position.y < 0 || position.z < 0 || position.z >= dimensions.z ||
                position.x > dimensions.x - point.unitSize || position.y > dimensions.y - point.unitSize)
                throw Exception(describePoint(terrain, block, point) + ": footprint is outside the MAP dimensions");
            for (size_t previous = 0; previous < index; ++previous)
            {
                const auto &other = rule.points[previous];
                if (position.z == other.position.z && position.x < other.position.x + other.unitSize &&
                    position.x + point.unitSize > other.position.x && position.y < other.position.y + other.unitSize &&
                    position.y + point.unitSize > other.position.y)
                    throw Exception(describePoint(terrain, block, point) +
                                    ": footprint overlaps another custom point at " + other.source);
            }
        }
    }
}  // namespace OpenXcom::TerrainSpawnRules
