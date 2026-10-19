#pragma once

namespace OpenXcom
{
    class AlienDeployment;
    class MissionSite;
    class Mod;
    class RuleTerrain;
    class Texture;

    namespace GlobalTerrain
    {
        /// Selects geographic terrain for mission sites when enabled and no explicit terrain takes precedence.
        /// Returns the existing terrain otherwise, including when no mission site is supplied.
        RuleTerrain *selectTerrain(Mod &mod, const AlienDeployment &deployment, RuleTerrain *selectedTerrain,
                                   Texture *globeTexture, MissionSite *missionSite);
    }  // namespace GlobalTerrain
}  // namespace OpenXcom
