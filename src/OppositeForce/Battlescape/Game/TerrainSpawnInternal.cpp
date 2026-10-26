#include "TerrainSpawnInternal.h"

#include <queue>

#include "../../../Battlescape/BattlescapeGame.h"
#include "../../../Battlescape/Pathfinding.h"
#include "../../../Mod/Mod.h"
#include "../../../Savegame/SavedBattleGame.h"
#include "../../../Savegame/Tile.h"

namespace OpenXcom::TerrainSpawnInternal
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
        {
            for (int y = 0; y < unit.getArmor()->getSize(); ++y)
            {
                const auto *tile = save.getTile(position + Position(x, y, 0));
                // setUnitPosition can raise a unit standing on a -24 tile. Custom coordinates must be exact.
                if (!tile || tile->getTerrainLevel() == -24 ||
                    tile->getTUCost(O_FLOOR, MT_WALK) == Pathfinding::INVALID_MOVE_COST)
                {
                    return false;
                }
            }
        }
        return save.setUnitPosition(&unit, position, true);
    }

    std::vector<unsigned char> calculateReachablePositions(SavedBattleGame &save, BattleUnit &unit,
                                                           const Position &start)
    {
        std::vector<unsigned char> reachable(save.getMapSizeXYZ(), 0);
        if (!canStandAtPosition(save, unit, start))
        {
            return reachable;
        }
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
                if (step.cost.time == Pathfinding::INVALID_MOVE_COST || !save.getTile(step.pos))
                {
                    continue;
                }
                const int index = save.getTileIndex(step.pos);
                if (visited[index])
                {
                    continue;
                }
                visited[index] = 1;
                // Normal walking can enter a transient falling/stair position before reaching a floor.
                // Traverse those legal steps, but only expose stable footprints as destinations.
                if (canStandAtPosition(save, unit, step.pos))
                {
                    reachable[index] = 1;
                }
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
        {
            for (int x = 0; x <= save.getMapSizeX() - unitSize; ++x)
            {
                const Position position(x, y, 0);
                if (reachable[save.getTileIndex(position)] &&
                    (x + unitSize <= origin.x || x >= origin.x + dimensions.x || y + unitSize <= origin.y ||
                     y >= origin.y + dimensions.y))
                {
                    return true;
                }
            }
        }
        return false;
    }

}  // namespace OpenXcom::TerrainSpawnInternal
