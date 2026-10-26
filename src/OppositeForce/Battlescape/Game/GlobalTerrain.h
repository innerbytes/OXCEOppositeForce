#pragma once

namespace OpenXcom
{
    class AlienDeployment;

    class MissionSite;

    class Mod;

    class RuleTerrain;

    class Texture;
}  // namespace OpenXcom

namespace OppositeForce::GlobalTerrain
{
    /// Selects geographic terrain for mission sites when enabled and no explicit terrain takes precedence.
    /// Returns the existing terrain otherwise, including when no mission site is supplied.
    OpenXcom::RuleTerrain *selectTerrain(OpenXcom::Mod &mod, const OpenXcom::AlienDeployment &deployment,
                                         OpenXcom::RuleTerrain *selectedTerrain, OpenXcom::Texture *globeTexture,
                                         OpenXcom::MissionSite *missionSite);
}  // namespace OppositeForce::GlobalTerrain
