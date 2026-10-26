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
#include "TerrainSpawnFixtures.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OppositeForce;
using namespace OppositeForce::Tests;
using namespace OppositeForce::TerrainSpawnPoints;

TEST_CASE("TerrainSpawnPoints_RepeatedFragmentPlacements_AppliesOffsetsWithoutChangingOriginalNodes",
          "[terrain-spawns][nodes]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.setPoints({createPoint(Position(2, 2, 0))});
    auto &original = fixture.addNode(Position(1, 1, 0));
    original.getNodeLinks()->push_back(-1);

    // Act
    addSpawnNodes(fixture.rules.getMod(), fixture.save,
                  {{&fixture.getBlock(), Position(0, 0, 0), 3}, {&fixture.getBlock(), Position(10, 0, 0), 4}});

    // Assert
    REQUIRE(original.getID() == 0);
    REQUIRE(original.getNodeLinks()->front() == -1);
    REQUIRE(fixture.save.getNodes()->size() == 3);
    REQUIRE(fixture.save.getNodes()->at(1)->getPosition() == Position(2, 2, 0));
    REQUIRE(fixture.save.getNodes()->at(2)->getPosition() == Position(12, 2, 0));
    REQUIRE(fixture.save.getNodes()->at(1)->getSegment() == 3);
    REQUIRE(fixture.save.getNodes()->at(2)->getSegment() == 4);
    REQUIRE(fixture.save.getNodes()->at(1)->getID() == 1);
}

TEST_CASE("TerrainSpawnPoints_VerticalFragmentPlacement_TranslatesSpawnFloor", "[terrain-spawns][nodes]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.setPart(Position(4, 4, 1), O_FLOOR, 1);
    fixture.setPoints({createPoint(Position(2, 2, 0))});

    // Act
    addSpawnNodes(fixture.rules.getMod(), fixture.save, {{&fixture.getBlock(), Position(2, 2, 1), 7}});

    // Assert
    REQUIRE(fixture.save.getNodes()->size() == 1);
    REQUIRE(fixture.save.getNodes()->front()->getPosition() == Position(4, 4, 1));
}

TEST_CASE("TerrainSpawnPoints_BlockedFootprintOrExit_OmitsCustomNode", "[terrain-spawns][nodes]")
{
    // Arrange
    TerrainFixture fixture;
    const int mode = GENERATE(0, 1, 2);
    CAPTURE(mode);
    if (mode == 0)
    {
        fixture.setPart(Position(4, 4, 0), O_OBJECT, 2);
    }
    if (mode == 1)
    {
        fixture.encloseRoom(false);
    }
    if (mode == 2)
    {
        fixture.initMap(10, 10, 2);
    }
    fixture.setPoints({createPoint(Position(2, 2, 0))});

    // Act
    addSpawnNodes(fixture.rules.getMod(), fixture.save,
                  {{&fixture.getBlock(), mode == 2 ? Position(0, 0, 0) : Position(2, 2, 0), 0}});

    // Assert
    REQUIRE(fixture.save.getNodes()->empty());
}

TEST_CASE("TerrainSpawnPoints_PatrolCandidates_LinksFiveNearestCompatibleReachableNodes", "[terrain-spawns][nodes]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.setPoints({createPoint(Position(5, 5, 0), 2)});
    for (int x = 1; x <= 7; ++x)
    {
        fixture.addNode(Position(x, 1, 0));
    }
    fixture.addNode(Position(4, 5, 0), 0, 1, Node::TYPE_SMALL);
    fixture.addNode(Position(5, 4, 0), 0, 1, Node::TYPE_FLYING);
    auto &blocked = fixture.addNode(Position(1, 10, 0));
    fixture.setPart(blocked.getPosition(), O_OBJECT, 2);

    // Act
    addSpawnNodes(fixture.rules.getMod(), fixture.save, {{&fixture.getBlock(), Position(0, 0, 0), 0}});

    // Assert
    auto &custom = *fixture.save.getNodes()->back();
    REQUIRE(*custom.getNodeLinks() == std::vector<int>{4, 3, 5, 2, 6});
    REQUIRE(custom.getFlags() == 1);
    for (const int id : *custom.getNodeLinks())
    {
        REQUIRE(fixture.save.getNodes()->at(id)->getNodeLinks()->back() == custom.getID());
    }
    REQUIRE(blocked.getNodeLinks()->empty());
}
