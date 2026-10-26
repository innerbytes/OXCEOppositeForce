#include "TerrainSpawnValidation.h"

#include <array>
#include <set>

#include "../../../Battlescape/BattlescapeGame.h"
#include "../../../Battlescape/Pathfinding.h"
#include "../../../Engine/Exception.h"
#include "../../../Engine/FileMap.h"
#include "../../../Engine/Language.h"
#include "../../../Engine/Logger.h"
#include "../../../Engine/RNG.h"
#include "../../../Mod/ExtraStrings.h"
#include "../../../Mod/MapBlock.h"
#include "../../../Mod/MapData.h"
#include "../../../Mod/MapDataSet.h"
#include "../../../Mod/Mod.h"
#include "../../../Mod/RuleCraft.h"
#include "../../../Mod/RuleTerrain.h"
#include "../../../Mod/RuleUfo.h"
#include "../../../Savegame/SavedBattleGame.h"
#include "../../../Savegame/Tile.h"
#include "../Game/TerrainSpawnInternal.h"
#include "TerrainSpawnRules.h"

namespace OpenXcom::TerrainSpawnValidation
{
    void attachRules(RuleTerrain &terrain)
    {
        for (auto *block : *terrain.getMapBlocks())
        {
            block->ofSetSpawnPoints({}, terrain.getName());
        }
        for (const auto &[name, rule] : terrain.ofGetSpawnRules())
        {
            bool found = false;
            for (auto *block : *terrain.getMapBlocks())
            {
                if (block->getName() == name)
                {
                    block->ofSetSpawnPoints(rule, terrain.getName());
                    found = true;
                }
            }
            if (!found)
            {
                throw Exception(rule.source + ": terrain " + terrain.getName() + ": unknown spawn fragment " + name);
            }
        }
    }

    Position loadFragment(Mod &mod, std::istream &stream, SavedBattleGame &save, RuleTerrain &terrain, MapBlock &block)
    {
        const auto fail = [&](const char *reason) {
            const auto &rule = block.ofGetSpawnPoints();
            const std::string context =
                rule.points.empty()
                    ? rule.source + ": terrain " + terrain.getName() + ", fragment " + block.getName()
                    : TerrainSpawnRules::describePoint(terrain.getName(), block.getName(), rule.points.front());
            throw Exception(context + ": " + reason);
        };
        if (terrain.getMapDataSets()->empty())
        {
            fail("Fragment terrain has no mapDataSets");
        }
        std::array<unsigned char, 3> header{};
        if (!stream.read(reinterpret_cast<char *>(header.data()), header.size()))
        {
            fail("Missing or truncated MAP header");
        }
        const Position dimensions(header[1], header[0], header[2]);
        if (dimensions.x != block.getSizeX() || dimensions.y != block.getSizeY() || dimensions.z == 0)
        {
            fail("MAP dimensions do not match the fragment rule");
        }
        TerrainSpawnRules::validateFootprints(terrain.getName(), block.getName(), block.ofGetSpawnPoints(), dimensions);
        save.initMap(dimensions.x + 4, dimensions.y + 4, dimensions.z);
        save.initUtilities(&mod);
        std::array<unsigned char, 4> parts{};
        for (int z = dimensions.z - 1; z >= 0; --z)
        {
            for (int y = 0; y < dimensions.y; ++y)
            {
                for (int x = 0; x < dimensions.x; ++x)
                {
                    if (!stream.read(reinterpret_cast<char *>(parts.data()), parts.size()))
                    {
                        fail("Truncated MAP tile data");
                    }
                    for (int part = O_FLOOR; part < O_MAX; ++part)
                    {
                        if (parts[part])
                        {
                            unsigned int id = parts[part];
                            int set = 0;
                            auto *data = terrain.getMapData(&id, &set);
                            save.getTile(Position(x + 2, y + 2, z))
                                ->setMapData(data, id, set, static_cast<TilePart>(part));
                        }
                    }
                }
            }
        }
        if (stream.peek() != std::char_traits<char>::eof())
        {
            fail("Excess MAP tile data");
        }
        block.setSizeZ(dimensions.z);
        return dimensions;
    }

    void validateFragment(Mod &mod, SavedBattleGame &save, const MapBlock &block)
    {
        const auto &rule = block.ofGetSpawnPoints();
        const Position dimensions(block.getSizeX(), block.getSizeY(), block.getSizeZ());
        TerrainSpawnRules::validateFootprints(block.ofGetSpawnTerrain(), block.getName(), rule, dimensions);
        for (const auto &point : rule.points)
        {
            TerrainSpawnInternal::WalkingProbe probe(mod, point.unitSize);
            const auto position = point.position + Position(2, 2, 0);
            if (!TerrainSpawnInternal::canStandAtPosition(save, probe.getUnit(), position))
            {
                throw Exception(TerrainSpawnRules::describePoint(block.ofGetSpawnTerrain(), block.getName(), point) +
                                ": footprint cannot stand on this tile (floor, wall, object, or level)");
            }
            const auto reachable = TerrainSpawnInternal::calculateReachablePositions(save, probe.getUnit(), position);
            if (!TerrainSpawnInternal::hasGroundExit(save, reachable, Position(2, 2, 0), dimensions, point.unitSize))
            {
                throw Exception(TerrainSpawnRules::describePoint(block.ofGetSpawnTerrain(), block.getName(), point) +
                                ": no walking route to the fragment's ground-level exterior");
            }
        }
    }

    void validateAllTerrains(Mod &mod)
    {
        struct SeedGuard
        {
            uint64_t seed = RNG::getSeed();

            ~SeedGuard()
            {
                RNG::setSeed(seed);
            }
        } guard;

        std::set<RuleTerrain *> terrains;
        for (const auto &name : mod.getTerrainList())
        {
            terrains.insert(mod.getTerrain(name));
        }
        for (const auto &name : mod.getCraftsList())
        {
            terrains.insert(mod.getCraft(name)->getBattlescapeTerrainData());
        }
        for (const auto &name : mod.getUfosList())
        {
            terrains.insert(mod.getUfo(name)->getBattlescapeTerrainData());
        }
        for (auto *terrain : terrains)
        {
            if (!terrain || terrain->ofGetSpawnRules().empty())
            {
                continue;
            }
            try
            {
                attachRules(*terrain);
                bool hasPoints = false;
                for (const auto &[name, rule] : terrain->ofGetSpawnRules())
                {
                    hasPoints |= !rule.points.empty();
                }
                if (!hasPoints)
                {
                    continue;
                }
                for (auto *set : *terrain->getMapDataSets())
                {
                    set->loadData(mod.getMCDPatch(set->getName()));
                }
                // SavedBattleGame initializes a hit log even in preview mode. Supply its unused strings
                // locally; validation must not depend on the user's selected language or log missing text.
                Language language;
                ExtraStrings strings;
                for (const auto *key : {"STR_HIT_LOG_NEW_TURN", "STR_HIT_LOG_REACTION_FIRE", "STR_HIT_LOG_NEW_BULLET",
                                        "STR_HIT_LOG_NO_DAMAGE", "STR_HIT_LOG_SMALL_DAMAGE", "STR_HIT_LOG_BIG_DAMAGE"})
                {
                    strings.getStrings()->emplace(key, "");
                }
                language.loadRule({{"en-US", &strings}}, "en-US");
                // Local tiles borrow MCD objects. Do not register shared datasets in the temporary save,
                // whose destructor would otherwise unload resources owned by the mod.
                // Tile caches borrow a dataset's sprite table even when no map is ever rendered.
                MapData openFloor(terrain->getMapDataSets()->empty() ? nullptr : terrain->getMapDataSets()->front());
                openFloor.setTUCosts(Pathfinding::DEFAULT_MOVE_COST, Pathfinding::DEFAULT_MOVE_COST,
                                     Pathfinding::DEFAULT_MOVE_COST);
                MapData *groundFloor = &openFloor;
                for (auto *set : *terrain->getMapDataSets())
                {
                    if (set->getName() == "BLANKS" && set->getSize() > 1)
                    {
                        groundFloor = set->getObject(1);
                    }
                }
                for (auto *block : *terrain->getMapBlocks())
                {
                    if (block->ofGetSpawnPoints().points.empty())
                    {
                        continue;
                    }
                    const auto &point = block->ofGetSpawnPoints().points.front();
                    SavedBattleGame save(&mod, &language, true);
                    auto stream = [&] {
                        try
                        {
                            return FileMap::getIStream("MAPS/" + block->getName() + ".MAP");
                        }
                        catch (const Exception &error)
                        {
                            throw Exception(
                                TerrainSpawnRules::describePoint(terrain->getName(), block->getName(), point) + ": " +
                                error.what());
                        }
                    }();
                    loadFragment(mod, *stream, save, *terrain, *block);
                    for (int y = 0; y < save.getMapSizeY(); ++y)
                    {
                        for (int x = 0; x < save.getMapSizeX(); ++x)
                        {
                            auto *tile = save.getTile(Position(x, y, 0));
                            if (!tile->getMapData(O_FLOOR))
                            {
                                const bool inside =
                                    x >= 2 && x < block->getSizeX() + 2 && y >= 2 && y < block->getSizeY() + 2;
                                tile->setMapData(inside ? groundFloor : &openFloor, 0, 0, O_FLOOR);
                            }
                        }
                    }
                    validateFragment(mod, save, *block);
                }
            }
            catch (const Exception &error)
            {
                Log(LOG_ERROR) << "Custom terrain spawn validation failed: " << error.what();
                throw;
            }
        }
    }
}  // namespace OpenXcom::TerrainSpawnValidation
