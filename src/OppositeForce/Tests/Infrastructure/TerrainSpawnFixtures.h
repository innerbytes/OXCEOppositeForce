#pragma once

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
#include "Savegame/Node.h"
#include "Savegame/SavedBattleGame.h"
#include "Savegame/Tile.h"

namespace OpenXcom::OppositeForceTests
{
    static Language &getFixtureLanguage()
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

    static TerrainSpawnRules::Point createPoint(const Position &position, int size = 1, int rank = 0, int priority = 1)
    {
        return {position, size, rank, priority, "test-spawns.rul:3:5"};
    }

    static std::string createMapBytes(int height = 2)
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
}  // namespace OpenXcom::OppositeForceTests
