#include "LandingTextures.h"

#include <algorithm>
#include <numeric>
#include <sstream>
#include <vector>

#include "../../Engine/Logger.h"
#include "../../Engine/RNG.h"
#include "../../Geoscape/Globe.h"
#include "../../Mod/RuleAlienMission.h"
#include "../../Mod/RuleRegion.h"
#include "../../Mod/RuleUfo.h"
#include "../../Savegame/Ufo.h"

namespace OpenXcom::LandingTextures
{
    struct Selection
    {
        bool found = false;
        std::pair<double, double> position{};
        int area = -1;
    };

    static Selection trySelectLandingPoint(const Globe &globe, const RuleRegion &region, size_t zone, int initialArea,
                                           int fakeWaterChance, bool forMissionSite,
                                           const std::vector<int> &excludedTextures)
    {
        const auto &areas = region.getMissionZones().at(zone).areas;
        if (areas.empty()) return {};
        if (initialArea < 0) initialArea = RNG::generate(0, areas.size() - 1);
        const bool allowCityLanding = !forMissionSite && areas.front().isPoint();
        const bool wantsFakeWater = !allowCityLanding && RNG::percent(fakeWaterChance);
        std::vector<int> remainingAreas(areas.size());
        std::iota(remainingAreas.begin(), remainingAreas.end(), 0);
        remainingAreas.erase(remainingAreas.begin() + initialArea);

        int selectedArea = initialArea;
        while (true)
        {
            const int attempts = areas.at(selectedArea).isPoint() ? 1 : 100;
            for (int attempt = 0; attempt < attempts; ++attempt)
            {
                const auto position = region.getRandomPoint(zone, selectedArea);
                if (!allowCityLanding &&
                    (!globe.insideLand(position.first, position.second) ||
                     (!forMissionSite && !region.insideRegion(position.first, position.second)) ||
                     globe.insideFakeUnderwaterTexture(position.first, position.second) != wantsFakeWater))
                    continue;

                int texture, shade;
                globe.getPolygonTextureAndShade(position.first, position.second, &texture, &shade);
                if (std::find(excludedTextures.begin(), excludedTextures.end(), texture) != excludedTextures.end())
                    continue;
                return {true, position, selectedArea};
            }
            if (remainingAreas.empty()) return {};
            const int next = RNG::generate(0, remainingAreas.size() - 1);
            selectedArea = remainingAreas[next];
            remainingAreas.erase(remainingAreas.begin() + next);
        }
    }

    std::pair<double, double> selectLandingPoint(const Globe &globe, const RuleRegion &region, size_t zone,
                                                 int initialArea, const Ufo &ufo, const RuleAlienMission &missionRule,
                                                 bool &selectionFailed, int &missionSiteArea)
    {
        const auto selection =
            trySelectLandingPoint(globe, region, zone, initialArea, ufo.getRules()->getFakeWaterLandingChance(),
                                  initialArea != -1, missionRule.ofGetExcludedLandingTextures());
        selectionFailed = !selection.found;
        if (selection.found)
        {
            if (initialArea != -1) missionSiteArea = selection.area;
            return selection.position;
        }
        std::ostringstream exclusions;
        for (int texture : missionRule.ofGetExcludedLandingTextures()) exclusions << texture << ' ';
        Log(LOG_WARNING) << "Mission: " << missionRule.getType() << " Region: " << region.getType() << " zone: " << zone
                         << " no permissible landing point found; excluded textures: [" << exclusions.str()
                         << "]; skipping UFO landing.";
        return {};
    }

    int findLandingArea(const RuleRegion &region, size_t zone, const std::pair<double, double> &position,
                        int preferredArea)
    {
        const auto &areas = region.getMissionZones().at(zone).areas;
        const auto containsPoint = [&](const MissionArea &area) {
            return position.first >= std::min(area.lonMin, area.lonMax) &&
                   position.first <= std::max(area.lonMin, area.lonMax) &&
                   position.second >= std::min(area.latMin, area.latMax) &&
                   position.second <= std::max(area.latMin, area.latMax);
        };
        // Another UFO in this mission may have selected a different area in the meantime.
        if (containsPoint(areas.at(preferredArea))) return preferredArea;
        for (size_t index = 0; index < areas.size(); ++index)
            if (containsPoint(areas[index])) return index;
        return preferredArea;
    }
}  // namespace OpenXcom::LandingTextures
