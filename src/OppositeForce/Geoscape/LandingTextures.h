#pragma once

#include <cstddef>
#include <utility>

namespace OpenXcom
{
    class Globe;
    class RuleAlienMission;
    class RuleRegion;
    class Ufo;

    namespace LandingTextures
    {
        /**
         * Selects landing coordinates on a permitted globe texture, respecting the UFO's surface requirements.
         * Searches the initial area first, then the other areas in the same zone, with bounded retries.
         * Logs a warning when no suitable point is found.
         * @param globe Globe used to check land, fake water, and actual texture IDs.
         * @param region Region providing the mission areas and ordinary landing boundaries.
         * @param zone Valid mission-zone index within the region.
         * @param initialArea Valid area index for a mission-site landing, or -1 for an ordinary UFO landing.
         * Mission-site landings may lie outside the region boundary; ordinary landings retain the city-point exception.
         * @param ufo UFO whose rules supply the fake-water landing chance.
         * @param missionRule Mission rules supplying the excluded textures and mission ID for diagnostics.
         * @param[out] selectionFailed Set to true when the search fails, or false on success.
         * @param[in,out] missionSiteArea Updated to the selected area on a successful mission-site landing.
         * Unchanged for ordinary landings and failed searches.
         * @return Longitude and latitude in radians, or {0, 0} on failure. Check selectionFailed before using them.
         */
        std::pair<double, double> selectLandingPoint(const Globe &globe, const RuleRegion &region, size_t zone,
                                                     int initialArea, const Ufo &ufo,
                                                     const RuleAlienMission &missionRule, bool &selectionFailed,
                                                     int &missionSiteArea);

        /**
         * Finds the area containing an arriving UFO's coordinates so its site retains the correct area metadata.
         * @param region Region providing the mission areas.
         * @param zone Valid mission-zone index within the region.
         * @param position Longitude and latitude in radians.
         * @param preferredArea Valid area index to prefer when areas overlap, and to return if none contains the point.
         * @return preferredArea if it contains the point; otherwise the first containing area, or preferredArea if
         * none.
         */
        int findLandingArea(const RuleRegion &region, size_t zone, const std::pair<double, double> &position,
                            int preferredArea);
    }  // namespace LandingTextures
}  // namespace OpenXcom
