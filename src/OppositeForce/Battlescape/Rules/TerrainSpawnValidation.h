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

namespace OppositeForce::TerrainSpawnValidation
{
    /// Validates and attaches final rules, using resources from FileMap.
    void validateAllTerrains(OpenXcom::Mod &mod);

    /// Loads a standalone MAP into an initialized, padded battlescape. Also used by synthetic fixtures.
    OpenXcom::Position loadFragment(OpenXcom::Mod &mod, std::istream &stream, OpenXcom::SavedBattleGame &save,
                                    OpenXcom::RuleTerrain &terrain, OpenXcom::MapBlock &block);

    /// Resolves fragment names, attaching definitions without changing block selection.
    void attachRules(OpenXcom::RuleTerrain &terrain);

    /// Validates a fragment's points using a supplied padded map.
    void validateFragment(OpenXcom::Mod &mod, OpenXcom::SavedBattleGame &save, const OpenXcom::MapBlock &block);
}  // namespace OppositeForce::TerrainSpawnValidation
