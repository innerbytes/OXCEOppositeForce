#pragma once

#include <vector>

#include "../../Mod/Armor.h"
#include "../../Mod/Unit.h"
#include "../../Savegame/BattleUnit.h"

namespace OpenXcom
{
    class Mod;
    class SavedBattleGame;

    namespace TerrainSpawnPoints
    {
        /// Probe rules belong to this object, never to the mod or battle save.
        class WalkingProbe
        {
            std::unique_ptr<Armor> _armor;
            Unit _rules;
            BattleUnit _unit;

        public:
            WalkingProbe(Mod &mod, int size);
            BattleUnit &getUnit()
            {
                return _unit;
            }
        };

        bool canStandAtPosition(SavedBattleGame &save, BattleUnit &unit, const Position &position);
        std::vector<unsigned char> calculateReachablePositions(SavedBattleGame &save, BattleUnit &unit,
                                                               const Position &start);
        bool hasGroundExit(const SavedBattleGame &save, const std::vector<unsigned char> &reachable,
                           const Position &origin, const Position &dimensions, int unitSize);
    }  // namespace TerrainSpawnPoints
}  // namespace OpenXcom
