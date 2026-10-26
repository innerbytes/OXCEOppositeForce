#include "MissionCompletionResult.h"

#include "../../Battlescape/BattlescapeGame.h"
#include "../../Battlescape/DebriefingState.h"
#include "../../Engine/Logger.h"
#include "../../Savegame/SavedBattleGame.h"

namespace OpenXcom::MissionCompletionResult
{
    bool shouldScriptedMissionResultApplyOnEndTurn(SavedBattleGame &battle, const BattlescapeTally &tally,
                                                   bool allEnemiesNeutralized)
    {
        // Squad defeat must use native loss handling, even when the script requests success.
        if (tally.liveSoldiers <= 0)
        {
            return false;
        }

        // An already-expired deployment timer takes priority and applies its configured outcome.
        if (battle.getTurnLimit() > 0 && battle.getTurn() > battle.getTurnLimit())
        {
            return false;
        }

        // Manual mode lets the script finish regardless of enemy elimination or ordinary objectives.
        if (battle.ofGetMissionCompletion().isManualCompletion())
        {
            return true;
        }

        // If manual completion is disabled and all objectives are destroyed, prefer native mission completion.
        if (battle.getObjectiveType() == MUST_DESTROY)
        {
            return !battle.allObjectivesDestroyed();
        }

        // If manual completion is disabled and this is a VIP escort mission (the 2 conditions just detect it's vip
        // protection mission), let the scripted result decide the outcome, so, if asked by the script, we can return
        // earlier than the VIP conditions are achieved
        if (battle.getVIPSurvivalPercentage() > 0 && battle.getVIPEscapeType() != ESCAPE_NONE)
        {
            return true;
        }

        // If manual completion is disabled and no enemies remain, prefer native mission completion.
        if (tally.liveAliens == 0)
        {
            return false;
        }

        // If manual completion is disabled and native logic already decided all enemies were neutralized, prefer native
        // mission completion.
        if (allEnemiesNeutralized)
        {
            return false;
        }

        // If manual completion is disabled and no native ending is due, apply the scripted result before turn effects.
        return true;
    }

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
