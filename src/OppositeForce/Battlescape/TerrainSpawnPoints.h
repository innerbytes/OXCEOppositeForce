#pragma once

#include <istream>
#include <vector>

#include "../Mod/TerrainSpawnRules.h"

namespace OpenXcom
{
    class BattleUnit;
    class MapBlock;
    class Mod;
    class Node;
    class RuleTerrain;
    class SavedBattleGame;

    namespace TerrainSpawnPoints
    {
        struct Placement
        {
            const MapBlock *block;
            Position offset;
            int segment;
        };

        /// Validates and attaches final rules, using resources from FileMap.
        void validateAllTerrains(Mod &mod);
        /// Loads a standalone MAP into an initialized, padded battlescape. Also used by synthetic fixtures.
        Position loadFragment(Mod &mod, std::istream &stream, SavedBattleGame &save, RuleTerrain &terrain,
                              MapBlock &block);
        /// Resolves fragment names, attaching definitions without changing block selection.
        void attachRules(RuleTerrain &terrain);
        /// Validates a fragment's points using a supplied padded map.
        void validateFragment(Mod &mod, SavedBattleGame &save, const MapBlock &block);
        /// Appends safe custom nodes only after all ordinary nodes and overlays have loaded.
        void addSpawnNodes(Mod &mod, SavedBattleGame &save, const std::vector<Placement> &placements);
        /// Applies custom size eligibility to spawn and patrol candidates.
        bool canUseNode(const Node &node, const BattleUnit &unit);
        /// Keeps small-unit spawns off accepted custom 2x2 footprints, across all ranks.
        std::vector<Position> getReservedFootprints(SavedBattleGame &save, const BattleUnit &unit);
        bool canSpawnAtNode(SavedBattleGame &save, const Node &node, const BattleUnit &unit,
                            const std::vector<Position> &reservations);
    }  // namespace TerrainSpawnPoints
}  // namespace OpenXcom
