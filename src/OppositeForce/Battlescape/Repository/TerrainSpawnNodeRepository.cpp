#include "TerrainSpawnNodeRepository.h"

#include "../../../Savegame/Node.h"

namespace OpenXcom::TerrainSpawnNodeRepository
{
    void load(Node &node, const YAML::YamlNodeReader &reader)
    {
        node.ofSetSpawnSize(reader["ofSpawnSize"].readVal(0));
    }

    void save(const Node &node, YAML::YamlNodeWriter writer)
    {
        if (node.ofGetSpawnSize())
        {
            writer.write("ofSpawnSize", node.ofGetSpawnSize());
        }
    }
}  // namespace OpenXcom::TerrainSpawnNodeRepository
