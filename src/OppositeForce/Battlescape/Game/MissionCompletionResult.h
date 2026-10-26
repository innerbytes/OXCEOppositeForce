#pragma once

#include <string>
#include <vector>

#include "MissionCompletion.h"

namespace OpenXcom
{
    struct DebriefingStat;

    struct BattlescapeTally;

    class SavedBattleGame;

}  // namespace OpenXcom

/// Resolves mission outcomes and debriefing presentation.
namespace OpenXcom::MissionCompletionResult
{
    /// Checks whether a pending script result should finish the battle at the current turn boundary.
    /// Native squad defeat, expired timers, completed objectives and enemy elimination take priority when applicable.
    bool shouldScriptedMissionResultApplyOnEndTurn(SavedBattleGame &battle, const BattlescapeTally &tally,
                                                   bool allEnemiesNeutralized);

    /// Resolves native and scripted mission outcomes, with squad defeat taking priority over scripted success.
    bool resolveSuccess(bool isNativeSuccess, MissionCompletion::Result scriptResult, int survivorsCount);

    /// Returns the standard debriefing title for a resolved outcome and target type.
    /// Returns an empty string and logs an error for inconsistent or unknown input.
    std::string getDefaultDebriefingTitle(bool success, const std::string &target, bool lostCraft);

    /// Clears native objective quantities and scores. This is useful when applying scripted objective awards.
    void clearObjectiveAwards(const std::vector<DebriefingStat *> &stats, const std::string &objectiveCompleteText,
                              const std::string &objectiveFailedText);
}  // namespace OpenXcom::MissionCompletionResult
