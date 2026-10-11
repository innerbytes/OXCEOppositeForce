#pragma once

namespace OpenXcom
{
    class AlienDeployment;
    class Mod;
    class RuleTerrain;
    class Target;
    class Texture;

    namespace GlobalTerrain
    {
        /// Selects geographic terrain only when enabled and no explicit terrain takes precedence.
        /// Returns the existing terrain otherwise, leaving ordinary generation to its original caller.
        RuleTerrain *selectTerrain(Mod &mod, const AlienDeployment &deployment, RuleTerrain *selectedTerrain,
                                   Texture *globeTexture, Target *target);
    }  // namespace GlobalTerrain
}  // namespace OpenXcom
