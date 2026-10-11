#include <array>
#include <queue>
#include <set>

#include "../../Battlescape/BattlescapeGame.h"
#include "../../Battlescape/Pathfinding.h"
#include "../../Engine/Exception.h"
#include "../../Engine/FileMap.h"
#include "../../Engine/Language.h"
#include "../../Engine/Logger.h"
#include "../../Engine/RNG.h"
#include "../../Mod/ExtraStrings.h"
#include "../../Mod/MapBlock.h"
#include "../../Mod/MapData.h"
#include "../../Mod/MapDataSet.h"
#include "../../Mod/Mod.h"
#include "../../Mod/RuleCraft.h"
#include "../../Mod/RuleTerrain.h"
#include "../../Mod/RuleUfo.h"
#include "../../Savegame/SavedBattleGame.h"
#include "../../Savegame/Tile.h"
#include "TerrainSpawnInternal.h"
#include "TerrainSpawnPoints.h"

namespace OpenXcom::TerrainSpawnPoints
{
    WalkingProbe::WalkingProbe(Mod &mod, int size)
        : _armor(Armor::ofCreateSpawnProbe(size)),
          _rules("OF_SPAWN_PROBE"),
          _unit(&mod, &_rules, FACTION_NEUTRAL, 0, nullptr, _armor.get(), nullptr, 0, nullptr)
    {
    }

    bool canStandAtPosition(SavedBattleGame &save, BattleUnit &unit, const Position &position)
    {
        for (int x = 0; x < unit.getArmor()->getSize(); ++x)
            for (int y = 0; y < unit.getArmor()->getSize(); ++y)
            {
                const auto *tile = save.getTile(position + Position(x, y, 0));
                // setUnitPosition can raise a unit standing on a -24 tile. Custom coordinates must be exact.
                if (!tile || tile->getTerrainLevel() == -24 ||
                    tile->getTUCost(O_FLOOR, MT_WALK) == Pathfinding::INVALID_MOVE_COST)
                    return false;
            }
        return save.setUnitPosition(&unit, position, true);
    }

    std::vector<unsigned char> calculateReachablePositions(SavedBattleGame &save, BattleUnit &unit,
                                                           const Position &start)
    {
        std::vector<unsigned char> reachable(save.getMapSizeXYZ(), 0);
        if (!canStandAtPosition(save, unit, start)) return reachable;
        std::vector<unsigned char> visited(save.getMapSizeXYZ(), 0);
        std::queue<Position> pending;
        reachable[save.getTileIndex(start)] = 1;
        visited[save.getTileIndex(start)] = 1;
        pending.push(start);
        while (!pending.empty())
        {
            const auto position = pending.front();
            pending.pop();
            for (int direction = 0; direction < 10; ++direction)
            {
                const auto step = save.getPathfinding()->getTUCost(position, direction, &unit, nullptr, BAM_NORMAL);
                if (step.cost.time == Pathfinding::INVALID_MOVE_COST || !save.getTile(step.pos)) continue;
                const int index = save.getTileIndex(step.pos);
                if (visited[index]) continue;
                visited[index] = 1;
                // Normal walking can enter a transient falling/stair position before reaching a floor.
                // Traverse those legal steps, but only expose stable footprints as destinations.
                if (canStandAtPosition(save, unit, step.pos)) reachable[index] = 1;
                pending.push(step.pos);
            }
        }
        // Probes must not leave a dangling pointer in the engine's shared pathfinder.
        save.getPathfinding()->setUnit(nullptr);
        return reachable;
    }

    bool hasGroundExit(const SavedBattleGame &save, const std::vector<unsigned char> &reachable, const Position &origin,
                       const Position &dimensions, int unitSize)
    {
        for (int y = 0; y <= save.getMapSizeY() - unitSize; ++y)
            for (int x = 0; x <= save.getMapSizeX() - unitSize; ++x)
            {
                const Position position(x, y, 0);
                if (reachable[save.getTileIndex(position)] &&
                    (x + unitSize <= origin.x || x >= origin.x + dimensions.x || y + unitSize <= origin.y ||
                     y >= origin.y + dimensions.y))
                    return true;
            }
        return false;
    }

    void attachRules(RuleTerrain &terrain)
    {
        for (auto *block : *terrain.getMapBlocks()) block->ofSetSpawnPoints({}, terrain.getName());
        for (const auto &[name, rule] : terrain.ofGetSpawnRules())
        {
            bool found = false;
            for (auto *block : *terrain.getMapBlocks())
                if (block->getName() == name)
                {
                    block->ofSetSpawnPoints(rule, terrain.getName());
                    found = true;
                }
            if (!found)
                throw Exception(rule.source + ": terrain " + terrain.getName() + ": unknown spawn fragment " + name);
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
        if (terrain.getMapDataSets()->empty()) fail("Fragment terrain has no mapDataSets");
        std::array<unsigned char, 3> header{};
        if (!stream.read(reinterpret_cast<char *>(header.data()), header.size()))
            fail("Missing or truncated MAP header");
        const Position dimensions(header[1], header[0], header[2]);
        if (dimensions.x != block.getSizeX() || dimensions.y != block.getSizeY() || dimensions.z == 0)
            fail("MAP dimensions do not match the fragment rule");
        TerrainSpawnRules::validateFootprints(terrain.getName(), block.getName(), block.ofGetSpawnPoints(), dimensions);
        save.initMap(dimensions.x + 4, dimensions.y + 4, dimensions.z);
        save.initUtilities(&mod);
        std::array<unsigned char, 4> parts{};
        for (int z = dimensions.z - 1; z >= 0; --z)
            for (int y = 0; y < dimensions.y; ++y)
                for (int x = 0; x < dimensions.x; ++x)
                {
                    if (!stream.read(reinterpret_cast<char *>(parts.data()), parts.size()))
                        fail("Truncated MAP tile data");
                    for (int part = O_FLOOR; part < O_MAX; ++part)
                        if (parts[part])
                        {
                            unsigned int id = parts[part];
                            int set = 0;
                            auto *data = terrain.getMapData(&id, &set);
                            save.getTile(Position(x + 2, y + 2, z))
                                ->setMapData(data, id, set, static_cast<TilePart>(part));
                        }
                }
        if (stream.peek() != std::char_traits<char>::eof()) fail("Excess MAP tile data");
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
            WalkingProbe probe(mod, point.unitSize);
            const auto position = point.position + Position(2, 2, 0);
            if (!canStandAtPosition(save, probe.getUnit(), position))
                throw Exception(TerrainSpawnRules::describePoint(block.ofGetSpawnTerrain(), block.getName(), point) +
                                ": footprint cannot stand on this tile (floor, wall, object, or level)");
            const auto reachable = calculateReachablePositions(save, probe.getUnit(), position);
            if (!hasGroundExit(save, reachable, Position(2, 2, 0), dimensions, point.unitSize))
                throw Exception(TerrainSpawnRules::describePoint(block.ofGetSpawnTerrain(), block.getName(), point) +
                                ": no walking route to the fragment's ground-level exterior");
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
        for (const auto &name : mod.getTerrainList()) terrains.insert(mod.getTerrain(name));
        for (const auto &name : mod.getCraftsList()) terrains.insert(mod.getCraft(name)->getBattlescapeTerrainData());
        for (const auto &name : mod.getUfosList()) terrains.insert(mod.getUfo(name)->getBattlescapeTerrainData());
        for (auto *terrain : terrains)
        {
            if (!terrain || terrain->ofGetSpawnRules().empty()) continue;
            try
            {
                attachRules(*terrain);
                bool hasPoints = false;
                for (const auto &[name, rule] : terrain->ofGetSpawnRules()) hasPoints |= !rule.points.empty();
                if (!hasPoints) continue;
                for (auto *set : *terrain->getMapDataSets()) set->loadData(mod.getMCDPatch(set->getName()));
                // SavedBattleGame initializes a hit log even in preview mode. Supply its unused strings
                // locally; validation must not depend on the user's selected language or log missing text.
                Language language;
                ExtraStrings strings;
                for (const auto *key : {"STR_HIT_LOG_NEW_TURN", "STR_HIT_LOG_REACTION_FIRE", "STR_HIT_LOG_NEW_BULLET",
                                        "STR_HIT_LOG_NO_DAMAGE", "STR_HIT_LOG_SMALL_DAMAGE", "STR_HIT_LOG_BIG_DAMAGE"})
                    strings.getStrings()->emplace(key, "");
                language.loadRule({{"en-US", &strings}}, "en-US");
                // Local tiles borrow MCD objects. Do not register shared datasets in the temporary save,
                // whose destructor would otherwise unload resources owned by the mod.
                // Tile caches borrow a dataset's sprite table even when no map is ever rendered.
                MapData openFloor(terrain->getMapDataSets()->empty() ? nullptr : terrain->getMapDataSets()->front());
                openFloor.setTUCosts(Pathfinding::DEFAULT_MOVE_COST, Pathfinding::DEFAULT_MOVE_COST,
                                     Pathfinding::DEFAULT_MOVE_COST);
                MapData *groundFloor = &openFloor;
                for (auto *set : *terrain->getMapDataSets())
                    if (set->getName() == "BLANKS" && set->getSize() > 1) groundFloor = set->getObject(1);
                for (auto *block : *terrain->getMapBlocks())
                {
                    if (block->ofGetSpawnPoints().points.empty()) continue;
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
}  // namespace OpenXcom::TerrainSpawnPoints
