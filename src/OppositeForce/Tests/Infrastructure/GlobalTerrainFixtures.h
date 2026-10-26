#pragma once

#include <algorithm>

#include "Engine/Exception.h"
#include "Fixtures.h"
#include "Mod/MapScript.h"
#include "Mod/Texture.h"
#include "OppositeForce/Battlescape/Game/GlobalTerrain.h"
#include "Savegame/MissionSite.h"

namespace OpenXcom::OppositeForceTests
{
    static void loadTexture(Texture &texture, Mod &mod, const std::string &yaml)
    {
        YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "test texture");
        texture.load(reader, &mod);
    }

    class GlobalTerrainFixture
    {
    public:
        RulesFixture rules;

        AlienDeployment &deployment;

        RuleTerrain &urban;

        RuleTerrain &forest;

        Texture siteTexture{1};

        Texture globeTexture{2};

        MissionSite site;

        explicit GlobalTerrainFixture(const std::string &yaml = "useGlobalTerrain: true")
            : deployment(rules.createDeployment("RESEARCH", yaml)),
              urban(rules.createTerrain("URBAN", "script: URBAN_SCRIPT")),
              forest(rules.createTerrain("FOREST",
                                         "script: FOREST_SCRIPT\nmapBlocks:\n"
                                         "  - {name: FOREST_A, width: 10, length: 10, groups: [0, 1]}\n"
                                         "  - {name: FOREST_B, width: 10, length: 10, groups: [0, 1]}\n")),
              site(nullptr, &deployment, nullptr)
        {
            loadTexture(siteTexture, rules.getMod(), "terrain: [{name: URBAN}]\nstartingCondition: SITE_CONDITION\n");
            loadTexture(globeTexture, rules.getMod(),
                        "terrain: [{name: FOREST}]\nstartingCondition: GLOBE_CONDITION\n");
            site.setTexture(1);
            site.setLongitude(Deg2Rad(20));
            site.setLatitude(Deg2Rad(10));
        }
    };

    static std::vector<MapBlock *> selectBlocks(MapScript &command, RuleTerrain &terrain)
    {
        std::vector<MapBlock *> blocks;
        command.init();
        for (int attempt = 0; attempt < 6; ++attempt)
        {
            blocks.push_back(command.getNextBlock(&terrain));
        }
        return blocks;
    }
}  // namespace OpenXcom::OppositeForceTests
