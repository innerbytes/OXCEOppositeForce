#pragma once

#include <memory>
#include <vector>

#include "../../../Mod/Armor.h"
#include "../../../Mod/Unit.h"
#include "../../../Savegame/BattleUnit.h"

namespace OpenXcom
{
    class Mod;

    class SavedBattleGame;
}  // namespace OpenXcom

namespace OppositeForce::TerrainSpawnInternal
{
    /// Probe rules belong to this object, never to the mod or battle save.
    class WalkingProbe
    {
        std::unique_ptr<OpenXcom::Armor> _armor;

        OpenXcom::Unit _rules;

        OpenXcom::BattleUnit _unit;

    public:
        WalkingProbe(OpenXcom::Mod &mod, int size);

        OpenXcom::BattleUnit &getUnit()
        {
            return _unit;
        }
    };

    bool canStandAtPosition(OpenXcom::SavedBattleGame &save, OpenXcom::BattleUnit &unit,
                            const OpenXcom::Position &position);

    std::vector<unsigned char> calculateReachablePositions(OpenXcom::SavedBattleGame &save, OpenXcom::BattleUnit &unit,
                                                           const OpenXcom::Position &start);

    bool hasGroundExit(const OpenXcom::SavedBattleGame &save, const std::vector<unsigned char> &reachable,
                       const OpenXcom::Position &origin, const OpenXcom::Position &dimensions, int unitSize);
}  // namespace OppositeForce::TerrainSpawnInternal
