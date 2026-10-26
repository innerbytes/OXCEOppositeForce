#include <filesystem>
#include <fstream>
#include <sstream>

#include "Engine/Exception.h"
#include "Engine/FileMap.h"
#include "Engine/ModInfo.h"
#include "Fixtures.h"
#include "Mod/MapBlock.h"
#include "Mod/MapData.h"
#include "OppositeForce/Battlescape/Game/TerrainSpawnInternal.h"
#include "OppositeForce/Battlescape/Game/TerrainSpawnPoints.h"
#include "OppositeForce/Battlescape/Rules/TerrainSpawnValidation.h"
#include "Savegame/Node.h"
#include "Savegame/SavedBattleGame.h"
#include "Savegame/Tile.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OppositeForce;
using namespace OppositeForce::Tests;

TEST_CASE("Node_CustomOrLegacySave_PreservesSizeAndLinksWithoutRequiringMetadata", "[terrain-spawns][save]")
{
    // Arrange
    const int size = GENERATE(0, 1, 2);
    CAPTURE(size);
    Node original(5, Position(3, 4, 0), 0, size == 1 ? Node::TYPE_SMALL : 0, 8, 1, 0, 12);
    original.ofSetSpawnSize(size);
    *original.getNodeLinks() = {1, 2, 3};
    YAML::YamlRootNodeWriter writer;
    Node restored;

    // Act
    original.save(writer);
    restored.load(writer.toReader());

    // Assert
    REQUIRE(restored.getID() == 5);
    REQUIRE(restored.getPosition() == Position(3, 4, 0));
    REQUIRE(restored.ofGetSpawnSize() == size);
    REQUIRE(*restored.getNodeLinks() == std::vector<int>{1, 2, 3});
    REQUIRE(static_cast<bool>(writer.toReader()["ofSpawnSize"]) == (size != 0));
}
