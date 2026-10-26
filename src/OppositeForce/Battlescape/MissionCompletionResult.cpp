#include "MissionCompletionResult.h"

#include "../../Battlescape/DebriefingState.h"
#include "../../Engine/Logger.h"

namespace OpenXcom::MissionCompletionResult
{
    bool resolveSuccess(bool isNativeSuccess, MissionCompletion::Result scriptResult, int survivorsCount)
    {
        if (scriptResult == MissionCompletion::Result::None)
        {
            return isNativeSuccess;
        }

        if (survivorsCount <= 0)
        {
            return false;
        }

        return scriptResult == MissionCompletion::Result::Success;
    }

    std::string getDefaultDebriefingTitle(bool success, const std::string &target, bool lostCraft)
    {
        if (success && lostCraft)
        {
            Log(LOG_ERROR) << "Mission completion: successful result with a lost craft for target '" << target << "'.";
            return {};
        }

        if (lostCraft) return "STR_CRAFT_IS_LOST";
        if (target == "STR_BASE") return success ? "STR_BASE_IS_SAVED" : "STR_BASE_IS_LOST";
        if (target == "STR_UFO") return success ? "STR_UFO_IS_RECOVERED" : "STR_UFO_IS_NOT_RECOVERED";
        if (target == "STR_ALIEN_BASE") return success ? "STR_ALIEN_BASE_DESTROYED" : "STR_ALIEN_BASE_STILL_INTACT";
        if (target == "STR_MISSION_SITE") return success ? "STR_ALIENS_DEFEATED" : "STR_TERROR_CONTINUES";

        // All expected target types are handled above; we should not reach here.
        Log(LOG_ERROR) << "Mission completion: unexpected debriefing target '" << target << "', success=" << success
                       << ", lostCraft=" << lostCraft << ".";

        return {};
    }

    void clearObjectiveAwards(const std::vector<DebriefingStat *> &stats, const std::string &objectiveCompleteText,
                              const std::string &objectiveFailedText)
    {
        for (auto *stat : stats)
        {
            if ((!objectiveCompleteText.empty() && stat->item == objectiveCompleteText) ||
                (!objectiveFailedText.empty() && stat->item == objectiveFailedText))
            {
                stat->qty = 0;
                stat->score = 0;
            }
        }
    }

}  // namespace OpenXcom::MissionCompletionResult
