#include <filesystem>
#include <fstream>
#include <sstream>

#include "Engine/Exception.h"
#include "Engine/FileMap.h"
#include "Engine/ModInfo.h"
#include "Fixtures.h"
#include "Mod/MapBlock.h"
#include "Mod/MapData.h"
#include "OppositeForce/Battlescape/TerrainSpawnInternal.h"
#include "OppositeForce/Battlescape/TerrainSpawnPoints.h"
#include "Savegame/Node.h"
#include "Savegame/SavedBattleGame.h"
#include "Savegame/Tile.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;
using namespace OpenXcom::TerrainSpawnPoints;

namespace
{
    Language &getFixtureLanguage()
    {
        static Language language;
        static const bool initialized = [] {
            loadLanguage(language,
                         "strings:\n"
                         "  STR_HIT_LOG_NEW_TURN: ''\n  STR_HIT_LOG_REACTION_FIRE: ''\n"
                         "  STR_HIT_LOG_NEW_BULLET: ''\n  STR_HIT_LOG_NO_DAMAGE: ''\n"
                         "  STR_HIT_LOG_SMALL_DAMAGE: ''\n  STR_HIT_LOG_BIG_DAMAGE: ''\n");
            return true;
        }();
        (void)initialized;
        return language;
    }

    class TerrainFixture
    {
    public:
        RulesFixture rules;
        MapDataSet &data;
        RuleTerrain &terrain;
        SavedBattleGame save;

        TerrainFixture()
            : data(rules.createMapDataSet("TEST")),
              terrain(rules.createTerrain("TEST",
                                          "mapDataSets: [TEST]\nmapBlocks:\n"
                                          "  - {name: BLOCK, width: 10, length: 10, height: 2, groups: [0, 9]}\n")),
              save(&rules.getMod(), &getFixtureLanguage(), true)
        {
            for (int index = 0; index < 4; ++index)
            {
                auto *object = new MapData(&data);
                object->setTUCosts(index == 2 ? 255 : 4, 4, 4);
                if (index == 3) object->setFlags(false, false, false, 0, false, true, false, false, false);
                data.getObjectsRaw()->push_back(object);
            }
            rules.createInventoryGround();
            initMap();
        }

        MapBlock &getBlock()
        {
            return *terrain.getMapBlocks()->front();
        }

        void initMap(int width = 24, int length = 24, int height = 2)
        {
            save.initMap(width, length, height);
            save.initUtilities(&rules.getMod());
            fillGround();
        }

        void fillGround()
        {
            for (int y = 0; y < save.getMapSizeY(); ++y)
                for (int x = 0; x < save.getMapSizeX(); ++x) setPart(Position(x, y, 0), O_FLOOR, 1);
        }

        void setPart(const Position &position, TilePart part, int id)
        {
            save.getTile(position)->setMapData(id < 0 ? nullptr : data.getObject(id), id, 0, part);
        }

        void loadLayer(const std::string &yaml)
        {
            auto buffer = yaml;
            const RawData bytes(buffer.data(), buffer.size(), +[](void *) {});
            YAML::YamlRootNodeReader reader(bytes, "test-spawns.rul");
            terrain.load(reader, &rules.getMod());
        }

        void setPoints(const std::vector<TerrainSpawnRules::Point> &points)
        {
            getBlock().ofSetSpawnPoints({points, "test-spawns.rul:1:1"}, "TEST");
        }

        void encloseRoom(bool door)
        {
            for (int coordinate = 3; coordinate <= 6; ++coordinate)
            {
                setPart(Position(3, coordinate, 0), O_WESTWALL, 2);
                setPart(Position(7, coordinate, 0), O_WESTWALL, 2);
                setPart(Position(coordinate, 3, 0), O_NORTHWALL, 2);
                setPart(Position(coordinate, 7, 0), O_NORTHWALL, 2);
            }
            if (door) setPart(Position(3, 4, 0), O_WESTWALL, 3);
        }

        Node &addNode(const Position &position, int rank = 0, int priority = 1, int type = 0)
        {
            auto *node = new Node(save.getNodes()->size(), position, 0, type, rank, 1, 0, priority);
            save.getNodes()->push_back(node);
            return *node;
        }
    };

    TerrainSpawnRules::Point createPoint(const Position &position, int size = 1, int rank = 0, int priority = 1)
    {
        return {position, size, rank, priority, "test-spawns.rul:3:5"};
    }

    std::string createMapBytes(int height = 2)
    {
        std::string bytes{char(10), char(10), char(height)};
        for (int z = height - 1; z >= 0; --z)
            for (int tile = 0; tile < 100; ++tile)
            {
                bytes.push_back(z == 0 ? 1 : 0);
                bytes.append(3, '\0');
            }
        return bytes;
    }

    // FileMap fixtures contain only authored synthetic files. No game or installed mod is loaded.
    class ResourceFixture
    {
        std::filesystem::path _directory;

    public:
        ResourceFixture(const std::string &mapBytes)
        {
            // std::filesystem has no unique-path facility; reserve a directory with bounded atomic attempts.
            for (int attempt = 0; attempt < 1000; ++attempt)
            {
                _directory =
                    std::filesystem::temp_directory_path() /
                    ("of-spawns-" + std::to_string(reinterpret_cast<uintptr_t>(this)) + "-" + std::to_string(attempt));
                if (std::filesystem::create_directory(_directory)) break;
                if (attempt == 999) throw std::runtime_error("Could not reserve synthetic resource directory");
            }
            const auto modPath = _directory / "synthetic";
            std::filesystem::create_directories(modPath / "MAPS");
            std::ofstream(modPath / "metadata.yml") << "id: synthetic\nname: Synthetic\nisMaster: true\n";
            std::ofstream(modPath / "MAPS" / "BLOCK.MAP", std::ios::binary).write(mapBytes.data(), mapBytes.size());
            FileMap::clear(true, false);
            FileMap::scanModDir(_directory.string() + "/", "", false);
            auto infos = FileMap::getModInfos();
            const auto found = infos.find("synthetic");
            if (found == infos.end()) throw std::runtime_error("Synthetic FileMap mod was not mapped");
            FileMap::setup({&found->second}, false);
        }

        ~ResourceFixture()
        {
            FileMap::clear(true, false);
            std::filesystem::remove_all(_directory);
        }
    };
}  // namespace

TEST_CASE("TerrainSpawnRules_CustomPoints_PreservesBlockSelectionAndLoadsDefaults", "[terrain-spawns][rules]")
{
    // Arrange
    TerrainFixture fixture;
    auto *original = &fixture.getBlock();

    // Act
    fixture.loadLayer("mapBlockSpawnPoints:\n  BLOCK:\n    - {position: [2, 3, 0], unitSize: 2}\n");
    attachRules(fixture.terrain);

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
    attachRules(fixture.terrain);

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
    attachRules(fixture.terrain);

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

TEST_CASE("TerrainSpawnRules_MalformedDefinitions_ThrowsValidationException", "[terrain-spawns][rules]")
{
    // Arrange
    const auto yaml = GENERATE("mapBlockSpawnPoints: []", "mapBlockSpawnPoints: {BLOCK: {}}",
                               "mapBlockSpawnPoints: {BLOCK: [{unitSize: 1}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2], unitSize: 1}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2, 0]}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2, 0], unitSize: 3}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2, 0], unitSize: 1, rank: -1}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2, 0], unitSize: 1, rank: 9}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2, 0], unitSize: 1, priority: 0}]}",
                               "mapBlockSpawnPoints: {BLOCK: [{position: [1, 2, 0], unitSize: 1, priority: 256}]}");
    CAPTURE(yaml);
    TerrainFixture fixture;

    // Act
    const auto load = [&] { fixture.loadLayer(yaml); };

    // Assert
    REQUIRE_THROWS(load());
}

TEST_CASE("TerrainSpawnValidation_UnknownFragment_ThrowsWithRuleSource", "[terrain-spawns][startup]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.loadLayer("mapBlockSpawnPoints: {TYPO: [{position: [1, 2, 0], unitSize: 1}]}");

    // Act
    const auto validate = [&] { validateAllTerrains(fixture.rules.getMod()); };

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
    const auto validate = [&] { validateFragment(fixture.rules.getMod(), fixture.save, fixture.getBlock()); };

    // Assert
    REQUIRE_THROWS_WITH(validate(), Catch::Matchers::ContainsSubstring("outside the MAP dimensions"));
}

TEST_CASE("TerrainSpawnValidation_OverlappingFootprints_ThrowsWithOverlapReason", "[terrain-spawns][startup]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.setPoints({createPoint(Position(1, 1, 0), 2), createPoint(Position(2, 2, 0))});

    // Act
    const auto validate = [&] { validateFragment(fixture.rules.getMod(), fixture.save, fixture.getBlock()); };

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
    const auto validate = [&] { validateAllTerrains(fixture.rules.getMod()); };

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
        loadFragment(fixture.rules.getMod(), stream, fixture.save, fixture.terrain, fixture.getBlock());
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
    if (mode == 0) fixture.setPart(Position(5, 5, 0), O_OBJECT, 2);
    if (mode == 2) fixture.setPart(Position(5, 4, 0), O_WESTWALL, 2);
    if (mode == 3) fixture.setPart(Position(4, 4, 0), O_FLOOR, -1);

    // Act
    const auto validate = [&] { validateFragment(fixture.rules.getMod(), fixture.save, fixture.getBlock()); };

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
    const auto validate = [&] { validateFragment(fixture.rules.getMod(), fixture.save, fixture.getBlock()); };

    // Assert
    if (valid)
        REQUIRE_NOTHROW(validate());
    else
        REQUIRE_THROWS_WITH(validate(), Catch::Matchers::ContainsSubstring("no walking route"));
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
    if (!connected) fixture.setPart(Position(4, 6, 1), O_NORTHWALL, 2);
    fixture.setPoints({createPoint(Position(2, 2, 1))});

    // Act
    const auto validate = [&] { validateFragment(fixture.rules.getMod(), fixture.save, fixture.getBlock()); };

    // Assert
    if (connected)
        REQUIRE_NOTHROW(validate());
    else
        REQUIRE_THROWS(validate());
}

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
    if (mode == 0) fixture.setPart(Position(4, 4, 0), O_OBJECT, 2);
    if (mode == 1) fixture.encloseRoom(false);
    if (mode == 2) fixture.initMap(10, 10, 2);
    fixture.setPoints({createPoint(Position(2, 2, 0))});

    // Act
    addSpawnNodes(fixture.rules.getMod(), fixture.save,
                  {{&fixture.getBlock(), mode == 2 ? Position(0, 0, 0) : Position(2, 2, 0), 0}});

    // Assert
    REQUIRE(fixture.save.getNodes()->empty());
}

TEST_CASE("SavedBattleGame_CustomSpawnSizesAndRanks_ReservesWholeLargeFootprintAcrossRanks", "[terrain-spawns][nodes]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.setPoints({createPoint(Position(2, 2, 0), 2, 0, 100), createPoint(Position(6, 6, 0), 1, 2, 20)});
    for (int x = 2; x < 4; ++x)
        for (int y = 2; y < 4; ++y) fixture.addNode(Position(x, y, 0), 2, 255);
    addSpawnNodes(fixture.rules.getMod(), fixture.save, {{&fixture.getBlock(), Position(0, 0, 0), 0}});
    WalkingProbe small(fixture.rules.getMod(), 1);
    WalkingProbe large(fixture.rules.getMod(), 2);

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
    WalkingProbe small(fixture.rules.getMod(), 1);

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
    WalkingProbe small(fixture.rules.getMod(), 1);
    WalkingProbe occupant(fixture.rules.getMod(), 1);
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
    WalkingProbe small(fixture.rules.getMod(), 1);
    WalkingProbe large(fixture.rules.getMod(), 2);

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
    WalkingProbe small(fixture.rules.getMod(), 1);

    // Act
    const bool canPlace = fixture.save.setUnitPosition(&small.getUnit(), Position(2, 2, 0), true);
    const auto *selected = fixture.save.getSpawnNode(1, &small.getUnit());

    // Assert
    REQUIRE(fixture.save.getNodes()->size() == 2);
    REQUIRE(canPlace);
    REQUIRE(selected == nullptr);
}

TEST_CASE("TerrainSpawnPoints_PatrolCandidates_LinksFiveNearestCompatibleReachableNodes", "[terrain-spawns][nodes]")
{
    // Arrange
    TerrainFixture fixture;
    fixture.setPoints({createPoint(Position(5, 5, 0), 2)});
    for (int x = 1; x <= 7; ++x) fixture.addNode(Position(x, 1, 0));
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
        REQUIRE(fixture.save.getNodes()->at(id)->getNodeLinks()->back() == custom.getID());
    REQUIRE(blocked.getNodeLinks()->empty());
}

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

TEST_CASE("TerrainSpawnPoints_UnconfiguredTerrain_PreservesOrdinaryNodesWithoutLoadingResources",
          "[terrain-spawns][nodes]")
{
    // Arrange
    TerrainFixture fixture;
    auto &ordinary = fixture.addNode(Position(2, 2, 0), 0, 5);
    WalkingProbe small(fixture.rules.getMod(), 1);
    WalkingProbe large(fixture.rules.getMod(), 2);

    // Act
    const auto validate = [&] { validateAllTerrains(fixture.rules.getMod()); };
    const auto *smallSpawn = fixture.save.getSpawnNode(0, &small.getUnit());
    const auto *largeSpawn = fixture.save.getSpawnNode(0, &large.getUnit());
    addSpawnNodes(fixture.rules.getMod(), fixture.save, {});

    // Assert
    REQUIRE_NOTHROW(validate());
    REQUIRE(smallSpawn == &ordinary);
    REQUIRE(largeSpawn == &ordinary);
    REQUIRE(fixture.save.getNodes()->size() == 1);
}
