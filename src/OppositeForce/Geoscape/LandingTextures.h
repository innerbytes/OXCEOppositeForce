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
        std::pair<double, double> selectLandingPoint(const Globe &globe, const RuleRegion &region, size_t zone,
                                                     int initialArea, const Ufo &ufo,
                                                     const RuleAlienMission &missionRule, bool &selectionFailed,
                                                     int &missionSiteArea);

        int findLandingArea(const RuleRegion &region, size_t zone, const std::pair<double, double> &position,
                            int preferredArea);
    }  // namespace LandingTextures
}  // namespace OpenXcom
