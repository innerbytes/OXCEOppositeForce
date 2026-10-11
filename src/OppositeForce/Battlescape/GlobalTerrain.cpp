#include "GlobalTerrain.h"

#include "../../Engine/Exception.h"
#include "../../Engine/Logger.h"
#include "../../Mod/AlienDeployment.h"
#include "../../Mod/Mod.h"
#include "../../Mod/Texture.h"

namespace OpenXcom::GlobalTerrain
{
    RuleTerrain *selectTerrain(Mod &mod, const AlienDeployment &deployment, RuleTerrain *selectedTerrain,
                               Texture *globeTexture, Target *target)
    {
        if (selectedTerrain || !deployment.ofGetUseGlobalTerrain() || !deployment.getTerrains().empty())
            return selectedTerrain;

        if (!globeTexture || globeTexture->getTerrain()->empty())
        {
            Log(LOG_WARNING) << "Map generator: deployment " << deployment.getType()
                             << " has no globe terrain definitions; using the first available terrain...";
            if (mod.getTerrainList().empty())
                throw Exception("Map generator encountered an error: No valid terrain found.");
            return mod.getTerrain(mod.getTerrainList().front(), true);
        }

        if (!target)
            throw Exception("Map generator encountered an error: No mission target for global terrain selection in " +
                            deployment.getType() + ".");

        auto *terrain = mod.getTerrain(globeTexture->getRandomTerrain(target), true);
        if (!terrain) throw Exception("Map generator encountered an error: No valid terrain found.");
        return terrain;
    }
}  // namespace OpenXcom::GlobalTerrain
