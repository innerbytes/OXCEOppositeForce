#pragma once

#include <vector>

#include "../../../Battlescape/Position.h"

namespace OpenXcom
{
    class BattleUnit;

    class MapBlock;

    class Mod;

    class Node;

    class SavedBattleGame;
}  // namespace OpenXcom

namespace OpenXcom::TerrainSpawnPoints
{
    struct Placement
    {
        const MapBlock *block;

        Position offset;

        int segment;
    };

    /// Appends safe custom nodes only after all ordinary nodes and overlays have loaded.
    void addSpawnNodes(Mod &mod, SavedBattleGame &save, const std::vector<Placement> &placements);

    /// Applies custom size eligibility to spawn and patrol candidates.
    bool canUseNode(const Node &node, const BattleUnit &unit);

    /// Keeps small-unit spawns off accepted custom 2x2 footprints, across all ranks.
    std::vector<Position> getReservedFootprints(SavedBattleGame &save, const BattleUnit &unit);

    bool canSpawnAtNode(SavedBattleGame &save, const Node &node, const BattleUnit &unit,
                        const std::vector<Position> &reservations);
}  // namespace OpenXcom::TerrainSpawnPoints
