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

namespace OppositeForce::TerrainSpawnPoints
{
    struct Placement
    {
        const OpenXcom::MapBlock *block;

        OpenXcom::Position offset;

        int segment;
    };

    /// Appends safe custom nodes only after all ordinary nodes and overlays have loaded.
    void addSpawnNodes(OpenXcom::Mod &mod, OpenXcom::SavedBattleGame &save, const std::vector<Placement> &placements);

    /// Applies custom size eligibility to spawn and patrol candidates.
    bool canUseNode(const OpenXcom::Node &node, const OpenXcom::BattleUnit &unit);

    /// Keeps small-unit spawns off accepted custom 2x2 footprints, across all ranks.
    std::vector<OpenXcom::Position> getReservedFootprints(OpenXcom::SavedBattleGame &save,
                                                          const OpenXcom::BattleUnit &unit);

    bool canSpawnAtNode(OpenXcom::SavedBattleGame &save, const OpenXcom::Node &node, const OpenXcom::BattleUnit &unit,
                        const std::vector<OpenXcom::Position> &reservations);
}  // namespace OppositeForce::TerrainSpawnPoints
