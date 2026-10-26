#include "MissionCompletion.h"

#include "../../Battlescape/DebriefingState.h"
#include "../../Engine/Logger.h"

namespace OpenXcom
{
    void MissionCompletion::setManualCompletion(bool manual)
    {
        _isManualCompletion = manual;
    }

    bool MissionCompletion::isManualCompletion() const
    {
        return _isManualCompletion;
    }

    void MissionCompletion::setResult(Result result)
    {
        _pendingResult = result;
    }

    MissionCompletion::Result MissionCompletion::getResult() const
    {
        return _pendingResult;
    }

    bool MissionCompletion::shouldSuppressAutomaticCompletion() const
    {
        return _isManualCompletion || getResult() != Result::None;
    }

    void MissionCompletion::reset()
    {
        _isManualCompletion = false;
        _pendingResult = Result::None;
    }

    bool MissionCompletion::resolveSuccess(bool isNativeSuccess, Result scriptResult, int survivorsCount)
    {
        if (scriptResult == Result::None)
        {
            return isNativeSuccess;
        }

        if (survivorsCount <= 0)
        {
            return false;
        }

        return scriptResult == Result::Success;
    }

    std::string MissionCompletion::getDefaultDebriefingTitle(bool success, const std::string &target, bool lostCraft)
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

    void MissionCompletion::clearObjectiveAwards(const std::vector<DebriefingStat *> &stats,
                                                 const std::string &objectiveCompleteText,
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

    void MissionCompletion::load(const YAML::YamlNodeReader &reader)
    {
        reset();
        const auto &node = reader["ofMissionCompletion"];
        if (node)
        {
            node.tryRead("isManualCompletion", _isManualCompletion);
            int pendingResult = 0;
            node.tryRead("pendingResult", pendingResult);
            _pendingResult = pendingResult >= 0 ? static_cast<Result>(pendingResult) : Result::None;
        }
    }

    void MissionCompletion::save(YAML::YamlNodeWriter writer) const
    {
        writer.setAsMap();
        auto node = writer["ofMissionCompletion"];
        node.setAsMap();
        node.write("isManualCompletion", _isManualCompletion);
        node.write("pendingResult", static_cast<int>(_pendingResult));
    }

}  // namespace OpenXcom
