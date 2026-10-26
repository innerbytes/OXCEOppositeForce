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

TEST_CASE("TerrainSpawnValidation_UnknownFragment_ThrowsWithRuleSource", "[terrain-spawns][startup]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.loadLayer("mapBlockSpawnPoints: {TYPO: [{position: [1, 2, 0], unitSize: 1}]}");

    // Act
    const auto validate = [&] { TerrainSpawnValidation::validateAllTerrains(fixture.rules.getMod()); };

    // Assert
    REQUIRE_THROWS_WITH(validate(), Catch::Matchers::ContainsSubstring("test-spawns.rul") &&
                                        Catch::Matchers::ContainsSubstring("unknown spawn fragment TYPO"));
}

TEST_CASE("TerrainSpawnValidation_OutOfBoundsFootprints_ThrowsWithDimensionReason", "[terrain-spawns][startup]")
{
    // Arrange
    TerrainFixture fixture;
    const auto position = GENERATE(Position(-1, 0, 0), Position(9, 0, 0), Position(0, 9, 0), Position(0, 0, 2));
    CAPTURE(position);
    fixture.setPoints({createPoint(position, 2)});

    // Act
    const auto validate = [&] {
        TerrainSpawnValidation::validateFragment(fixture.rules.getMod(), fixture.save, fixture.getBlock());
    };

    // Assert
    REQUIRE_THROWS_WITH(validate(), Catch::Matchers::ContainsSubstring("outside the MAP dimensions"));
}

TEST_CASE("TerrainSpawnValidation_OverlappingFootprints_ThrowsWithOverlapReason", "[terrain-spawns][startup]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.setPoints({createPoint(Position(1, 1, 0), 2), createPoint(Position(2, 2, 0))});

    // Act
    const auto validate = [&] {
        TerrainSpawnValidation::validateFragment(fixture.rules.getMod(), fixture.save, fixture.getBlock());
    };

    // Assert
    REQUIRE_THROWS_WITH(validate(), Catch::Matchers::ContainsSubstring("overlaps another custom point"));
}

TEST_CASE("TerrainSpawnValidation_ActualMapHeight_ReportsInvalidCoordinatesWithoutConsumingRandomness",
          "[terrain-spawns][startup]")
{
    // Arrange
    TerrainFixture fixture;
    const bool valid = GENERATE(true, false);
    CAPTURE(valid);
    fixture.loadLayer(valid ? "mapBlockSpawnPoints: {BLOCK: [{position: [2, 2, 0], unitSize: 2}]}"
                            : "mapBlockSpawnPoints: {BLOCK: [{position: [2, 2, 0], unitSize: 1}, "
                              "{position: [2, 2, 1], unitSize: 2}]}");
    ResourceFixture resources(createMapBytes(1));
    const auto seed = RNG::getSeed();

    // Act
    const auto validate = [&] { TerrainSpawnValidation::validateAllTerrains(fixture.rules.getMod()); };

    // Assert
    if (valid)
    {
        REQUIRE_NOTHROW(validate());
        REQUIRE(fixture.getBlock().getSizeZ() == 1);
        REQUIRE(fixture.data.getSize() == 4);  // temporary maps must not unload shared terrain objects
    }
    else
    {
        REQUIRE_THROWS_WITH(validate(), Catch::Matchers::ContainsSubstring("test-spawns.rul") &&
                                            Catch::Matchers::ContainsSubstring("fragment BLOCK") &&
                                            Catch::Matchers::ContainsSubstring("position [2, 2, 1]") &&
                                            !Catch::Matchers::ContainsSubstring("position [2, 2, 0]") &&
                                            Catch::Matchers::ContainsSubstring("unitSize 2") &&
                                            Catch::Matchers::ContainsSubstring("outside the MAP dimensions"));
    }
    REQUIRE(RNG::getSeed() == seed);
}

TEST_CASE("TerrainSpawnValidation_TruncatedOrInconsistentMap_ThrowsLoadingException", "[terrain-spawns][startup]")
{
    // Arrange
    TerrainFixture fixture;
    const auto bytes = GENERATE(std::string("\x0a\x0a", 2), std::string("\x09\x0a\x01", 3),
                                createMapBytes().substr(0, 30), createMapBytes() + "extra");
    std::istringstream stream(bytes);

    // Act
    const auto load = [&] {
        TerrainSpawnValidation::loadFragment(fixture.rules.getMod(), stream, fixture.save, fixture.terrain,
                                             fixture.getBlock());
    };

    // Assert
    REQUIRE_THROWS(load());
}

TEST_CASE("TerrainSpawnValidation_BlockedFootprintOrMissingFloor_ThrowsPlacementReason", "[terrain-spawns][paths]")
{
    // Arrange
    TerrainFixture fixture;
    const auto mode = GENERATE(0, 1, 2, 3);
    CAPTURE(mode);
    fixture.setPoints({createPoint(Position(2, 2, mode == 1 ? 1 : 0), 2)});
    if (mode == 0)
    {
        fixture.setPart(Position(5, 5, 0), O_OBJECT, 2);
    }
    if (mode == 2)
    {
        fixture.setPart(Position(5, 4, 0), O_WESTWALL, 2);
    }
    if (mode == 3)
    {
        fixture.setPart(Position(4, 4, 0), O_FLOOR, -1);
    }

    // Act
    const auto validate = [&] {
        TerrainSpawnValidation::validateFragment(fixture.rules.getMod(), fixture.save, fixture.getBlock());
    };

    // Assert
    REQUIRE_THROWS_WITH(validate(), Catch::Matchers::ContainsSubstring("footprint cannot stand"));
}

TEST_CASE("TerrainSpawnValidation_EnclosedRoomWithOptionalDoor_AcceptsOnlySizesWithWalkingExit",
          "[terrain-spawns][paths]")
{
    // Arrange
    TerrainFixture fixture;
    const auto [door, size, valid] =
        GENERATE(table<bool, int, bool>({{false, 1, false}, {false, 2, false}, {true, 1, true}, {true, 2, false}}));
    CAPTURE(door, size);
    fixture.encloseRoom(door);
    fixture.setPoints({createPoint(Position(2, 2, 0), size)});

    // Act
    const auto validate = [&] {
        TerrainSpawnValidation::validateFragment(fixture.rules.getMod(), fixture.save, fixture.getBlock());
    };

    // Assert
    if (valid)
    {
        REQUIRE_NOTHROW(validate());
    }
    else
    {
        REQUIRE_THROWS_WITH(validate(), Catch::Matchers::ContainsSubstring("no walking route"));
    }
}

TEST_CASE("TerrainSpawnValidation_UpperFloorPlatform_RequiresWalkingConnectionToGround", "[terrain-spawns][paths]")
{
    // Arrange
    TerrainFixture fixture;
    const bool connected = GENERATE(true, false);
    CAPTURE(connected);
    fixture.setPart(Position(4, 4, 1), O_FLOOR, 1);
    fixture.setPart(Position(4, 5, 1), O_FLOOR, 1);
    if (connected)
    {
        auto *stairs = new MapData(&fixture.data);
        stairs->setTUCosts(4, 4, 4);
        stairs->setTerrainLevel(-16);
        fixture.data.getObjectsRaw()->push_back(stairs);
        fixture.setPart(Position(4, 6, 0), O_OBJECT, 4);
        auto *step = new MapData(&fixture.data);
        step->setTUCosts(4, 4, 4);
        step->setTerrainLevel(-8);
        fixture.data.getObjectsRaw()->push_back(step);
        fixture.setPart(Position(4, 7, 0), O_OBJECT, 5);
    }
    // Without stairs, falling into open ground is also a legal normal walking route.
    // Enclose the upper platform to make the disconnected case genuinely inaccessible.
    for (int y = 4; y <= 5; ++y)
    {
        fixture.setPart(Position(4, y, 1), O_WESTWALL, 2);
        fixture.setPart(Position(5, y, 1), O_WESTWALL, 2);
    }
    fixture.setPart(Position(4, 4, 1), O_NORTHWALL, 2);
    if (!connected)
    {
        fixture.setPart(Position(4, 6, 1), O_NORTHWALL, 2);
    }
    fixture.setPoints({createPoint(Position(2, 2, 1))});

    // Act
    const auto validate = [&] {
        TerrainSpawnValidation::validateFragment(fixture.rules.getMod(), fixture.save, fixture.getBlock());
    };

    // Assert
    if (connected)
    {
        REQUIRE_NOTHROW(validate());
    }
    else
    {
        REQUIRE_THROWS(validate());
    }
}
