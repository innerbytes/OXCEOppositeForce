#pragma once

#include <istream>

#include "../../../Battlescape/Position.h"

namespace OpenXcom
{
    class MapBlock;

    class Mod;

    class RuleTerrain;

    class SavedBattleGame;
}  // namespace OpenXcom

namespace OpenXcom::TerrainSpawnValidation
{
    /// Validates and attaches final rules, using resources from FileMap.
    void validateAllTerrains(Mod &mod);

    /// Loads a standalone MAP into an initialized, padded battlescape. Also used by synthetic fixtures.
    Position loadFragment(Mod &mod, std::istream &stream, SavedBattleGame &save, RuleTerrain &terrain, MapBlock &block);

    /// Resolves fragment names, attaching definitions without changing block selection.
    void attachRules(RuleTerrain &terrain);

    /// Validates a fragment's points using a supplied padded map.
    void validateFragment(Mod &mod, SavedBattleGame &save, const MapBlock &block);
}  // namespace OpenXcom::TerrainSpawnValidation
