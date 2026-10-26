#pragma once

#include <string>
#include <vector>

#include "MissionCompletion.h"

namespace OpenXcom
{
    struct DebriefingStat;

}  // namespace OpenXcom

/// Resolves mission outcomes and debriefing presentation.
namespace OpenXcom::MissionCompletionResult
{
    bool resolveSuccess(bool isNativeSuccess, MissionCompletion::Result scriptResult, int survivorsCount);

    std::string getDefaultDebriefingTitle(bool success, const std::string &target, bool lostCraft);

    void clearObjectiveAwards(const std::vector<DebriefingStat *> &stats, const std::string &objectiveCompleteText,
                              const std::string &objectiveFailedText);
}  // namespace OpenXcom::MissionCompletionResult
