#pragma once

#include <cstddef>
#include <utility>
#include <vector>

namespace OpenXcom
{
    class Globe;
    class RuleRegion;

    namespace LandingTextures
    {
        struct Selection
        {
            bool found = false;
            std::pair<double, double> position{};
            int area = -1;
        };

        Selection selectLandingPoint(const Globe &globe, const RuleRegion &region, size_t zone, int initialArea,
                                     int fakeWaterChance, bool forMissionSite,
                                     const std::vector<int> &excludedTextures);

        int findLandingArea(const RuleRegion &region, size_t zone, const std::pair<double, double> &position,
                            int preferredArea);
    }  // namespace LandingTextures
}  // namespace OpenXcom
