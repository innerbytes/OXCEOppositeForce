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
using namespace OpenXcom::OppositeForceTests;
using namespace OpenXcom::TerrainSpawnPoints;

TEST_CASE("TerrainSpawnRules_CustomPoints_PreservesBlockSelectionAndLoadsDefaults", "[terrain-spawns][rules]")
{
    // Arrange
    TerrainFixture fixture;
    auto *original = &fixture.getBlock();

    // Act
    fixture.loadLayer("mapBlockSpawnPoints:\n  BLOCK:\n    - {position: [2, 3, 0], unitSize: 2}\n");
    TerrainSpawnValidation::attachRules(fixture.terrain);

    // Assert
    REQUIRE(fixture.terrain.getMapBlocks()->size() == 1);
    REQUIRE(&fixture.getBlock() == original);
    REQUIRE(original->isInGroup(0));
    REQUIRE(original->isInGroup(9));
    REQUIRE(original->getSizeX() == 10);
    REQUIRE(original->ofGetSpawnPoints().points.size() == 1);
    const auto &point = original->ofGetSpawnPoints().points.front();
    REQUIRE(point.unitSize == 2);
    REQUIRE(point.rank == 0);
    REQUIRE(point.priority == 1);
    REQUIRE(point.position == Position(2, 3, 0));
    REQUIRE(point.source.find("test-spawns.rul:3:") != std::string::npos);
}

TEST_CASE("TerrainSpawnRules_DuplicateFragmentNames_AttachesPointsToEveryMatchingBlock", "[terrain-spawns][rules]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.loadLayer("mapBlockSpawnPoints:\n  BLOCK:\n    - {position: [2, 3, 0], unitSize: 2}\n");
    fixture.loadLayer("addOnly: true\nmapBlocks: [{name: BLOCK, width: 10, length: 10}]\n");

    // Act
    TerrainSpawnValidation::attachRules(fixture.terrain);

    // Assert
    REQUIRE(fixture.terrain.getMapBlocks()->size() == 2);
    for (const auto *block : *fixture.terrain.getMapBlocks())
    {
        REQUIRE(block->ofGetSpawnPoints().points.size() == 1);
        const auto &point = block->ofGetSpawnPoints().points.front();
        REQUIRE(point.unitSize == 2);
        REQUIRE(point.rank == 0);
        REQUIRE(point.priority == 1);
        REQUIRE(point.position == Position(2, 3, 0));
        REQUIRE(point.source.find("test-spawns.rul:3:") != std::string::npos);
    }
}

TEST_CASE("TerrainSpawnRules_LayeredFragmentDefinitions_PreservesReplacesOrClearsInheritedPoints",
          "[terrain-spawns][rules]")
{
    // Arrange
    const auto [layer, count, position, size, rank, priority] =
        GENERATE(table<const char *, int, Position, int, int, int>({
            {"ambientVolume: 0.5\n", 1, Position(1, 2, 0), 1, 8, 255},
            {"mapBlockSpawnPoints:\n  BLOCK: [{position: [4, 5, 0], unitSize: 2}]\n", 1, Position(4, 5, 0), 2, 0, 1},
            {"mapBlockSpawnPoints: {BLOCK: []}\n", 0, Position(), 0, 0, 0},
        }));
    CAPTURE(layer);
    TerrainFixture fixture;
    fixture.loadLayer(
        "refNode:\n  mapBlockSpawnPoints:\n    BLOCK: [{position: [1, 2, 0], unitSize: 1, rank: 8, priority: 255}]\n");

    // Act
    fixture.loadLayer(layer);
    TerrainSpawnValidation::attachRules(fixture.terrain);

    // Assert
    const auto &points = fixture.getBlock().ofGetSpawnPoints().points;
    REQUIRE(points.size() == count);
    if (!points.empty())
    {
        REQUIRE(points.front().position == position);
        REQUIRE(points.front().unitSize == size);
        REQUIRE(points.front().rank == rank);
        REQUIRE(points.front().priority == priority);
    }
}

TEST_CASE("SavedBattleGame_CustomSpawnSizesAndRanks_ReservesWholeLargeFootprintAcrossRanks", "[terrain-spawns][nodes]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.setPoints({createPoint(Position(2, 2, 0), 2, 0, 100), createPoint(Position(6, 6, 0), 1, 2, 20)});
    for (int x = 2; x < 4; ++x)
        for (int y = 2; y < 4; ++y) fixture.addNode(Position(x, y, 0), 2, 255);
    addSpawnNodes(fixture.rules.getMod(), fixture.save, {{&fixture.getBlock(), Position(0, 0, 0), 0}});
    TerrainSpawnInternal::WalkingProbe small(fixture.rules.getMod(), 1);
    TerrainSpawnInternal::WalkingProbe large(fixture.rules.getMod(), 2);

    // Act
    const auto *smallRankZero = fixture.save.getSpawnNode(0, &small.getUnit());
    const auto *smallRankTwo = fixture.save.getSpawnNode(2, &small.getUnit());
    const auto *largeRankZero = fixture.save.getSpawnNode(0, &large.getUnit());
    const bool acceptsLarge = canUseNode(*fixture.save.getNodes()->at(5), large.getUnit());

    // Assert
    REQUIRE(smallRankZero == nullptr);
    REQUIRE(smallRankTwo != nullptr);
    REQUIRE(smallRankTwo->getPosition() == Position(6, 6, 0));
    REQUIRE(largeRankZero != nullptr);
    REQUIRE(largeRankZero->getPosition() == Position(2, 2, 0));
    REQUIRE_FALSE(acceptsLarge);
}

TEST_CASE("SavedBattleGame_EligibleSpawnPriorities_SelectsHighestPriorityNode", "[terrain-spawns][nodes]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.setPoints({createPoint(Position(6, 6, 0), 1, 2, 20)});
    addSpawnNodes(fixture.rules.getMod(), fixture.save, {{&fixture.getBlock(), Position(0, 0, 0), 0}});
    auto &higherPriority = fixture.addNode(Position(8, 8, 0), 2, 30);
    TerrainSpawnInternal::WalkingProbe small(fixture.rules.getMod(), 1);

    // Act
    const auto *selected = fixture.save.getSpawnNode(2, &small.getUnit());

    // Assert
    REQUIRE(selected == &higherPriority);
}

TEST_CASE("SavedBattleGame_OccupiedCustomSpawn_ReturnsNoAvailableNode", "[terrain-spawns][nodes]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.setPoints({createPoint(Position(6, 6, 0), 1, 2, 20)});
    addSpawnNodes(fixture.rules.getMod(), fixture.save, {{&fixture.getBlock(), Position(0, 0, 0), 0}});
    auto &higherPriority = fixture.addNode(Position(8, 8, 0), 2, 30);
    higherPriority.setDummy(true);
    TerrainSpawnInternal::WalkingProbe small(fixture.rules.getMod(), 1);
    TerrainSpawnInternal::WalkingProbe occupant(fixture.rules.getMod(), 1);
    const bool placed = fixture.save.setUnitPosition(&occupant.getUnit(), Position(6, 6, 0));

    // Act
    const auto *selected = fixture.save.getSpawnNode(2, &small.getUnit());
    occupant.getUnit().setTile(nullptr, &fixture.save);

    // Assert
    REQUIRE(placed);
    REQUIRE(selected == nullptr);
}

TEST_CASE("SavedBattleGame_CustomPatrolSizes_SelectsMatchingNodesWithoutRestrictingMovement", "[terrain-spawns][nodes]")
{
    // Arrange
    TerrainFixture fixture;
    auto &start = fixture.addNode(Position(1, 1, 0));
    fixture.setPoints({createPoint(Position(3, 3, 0), 2, 4), createPoint(Position(7, 7, 0), 1)});
    addSpawnNodes(fixture.rules.getMod(), fixture.save, {{&fixture.getBlock(), Position(0, 0, 0), 0}});
    TerrainSpawnInternal::WalkingProbe small(fixture.rules.getMod(), 1);
    TerrainSpawnInternal::WalkingProbe large(fixture.rules.getMod(), 2);

    // Act
    const auto *smallPatrol = fixture.save.getPatrolNode(false, &small.getUnit(), &start);
    const auto *largePatrol = fixture.save.getPatrolNode(false, &large.getUnit(), &start);
    const bool canMove = fixture.save.setUnitPosition(&small.getUnit(), Position(4, 4, 0), true);

    // Assert
    REQUIRE(smallPatrol != nullptr);
    REQUIRE(smallPatrol->getPosition() == Position(7, 7, 0));
    REQUIRE(largePatrol != nullptr);
    REQUIRE(largePatrol->getPosition() == Position(3, 3, 0));
    REQUIRE(canMove);
}

TEST_CASE("SavedBattleGame_OriginalNodeRaisedIntoLargeFootprint_RejectsSmallSpawn", "[terrain-spawns][nodes]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.addNode(Position(2, 2, 0), 1);
    auto *raised = new MapData(&fixture.data);
    raised->setTUCosts(4, 4, 4);
    raised->setTerrainLevel(-24);
    fixture.data.getObjectsRaw()->push_back(raised);
    fixture.setPart(Position(2, 2, 0), O_OBJECT, 4);
    for (int x = 2; x < 4; ++x)
        for (int y = 2; y < 4; ++y) fixture.setPart(Position(x, y, 1), O_FLOOR, 1);
    fixture.setPoints({createPoint(Position(2, 2, 1), 2)});
    addSpawnNodes(fixture.rules.getMod(), fixture.save, {{&fixture.getBlock(), Position(0, 0, 0), 0}});
    TerrainSpawnInternal::WalkingProbe small(fixture.rules.getMod(), 1);

    // Act
    const bool canPlace = fixture.save.setUnitPosition(&small.getUnit(), Position(2, 2, 0), true);
    const auto *selected = fixture.save.getSpawnNode(1, &small.getUnit());

    // Assert
    REQUIRE(fixture.save.getNodes()->size() == 2);
    REQUIRE(canPlace);
    REQUIRE(selected == nullptr);
}

TEST_CASE("TerrainSpawnPoints_UnconfiguredTerrain_PreservesOrdinaryNodesWithoutLoadingResources",
          "[terrain-spawns][nodes]")
{
    // Arrange
    TerrainFixture fixture;
    auto &ordinary = fixture.addNode(Position(2, 2, 0), 0, 5);
    TerrainSpawnInternal::WalkingProbe small(fixture.rules.getMod(), 1);
    TerrainSpawnInternal::WalkingProbe large(fixture.rules.getMod(), 2);

    // Act
    const auto validate = [&] { TerrainSpawnValidation::validateAllTerrains(fixture.rules.getMod()); };
    const auto *smallSpawn = fixture.save.getSpawnNode(0, &small.getUnit());
    const auto *largeSpawn = fixture.save.getSpawnNode(0, &large.getUnit());
    addSpawnNodes(fixture.rules.getMod(), fixture.save, {});

    // Assert
    REQUIRE_NOTHROW(validate());
    REQUIRE(smallSpawn == &ordinary);
    REQUIRE(largeSpawn == &ordinary);
    REQUIRE(fixture.save.getNodes()->size() == 1);
}
