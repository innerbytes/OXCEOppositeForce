#include "MissionCompletion.h"

#include "../../Mod/AlienDeployment.h"

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

    MissionCompletion::DebriefingOutcome MissionCompletion::getDebriefingOutcome(Result result,
                                                                                 const AlienDeployment *deployment,
                                                                                 const std::string &target,
                                                                                 int survivors, bool lostCraft)
    {
        DebriefingOutcome outcome{resolveSuccess(false, result, survivors) && !lostCraft, {}, {}, 0};
        if (deployment)
        {
            if (outcome.success)
            {
                deployment->getObjectiveCompleteInfo(outcome.objectiveText, outcome.objectiveScore, outcome.title);
            }
            else
            {
                deployment->getObjectiveFailedInfo(outcome.objectiveText, outcome.objectiveScore, outcome.title);
            }
        }
        if (outcome.title.empty())
        {
            if (lostCraft)
                outcome.title = "STR_CRAFT_IS_LOST";
            else if (target == "STR_BASE")
                outcome.title = outcome.success ? "STR_BASE_IS_SAVED" : "STR_BASE_IS_LOST";
            else if (target == "STR_UFO")
                outcome.title = outcome.success ? "STR_UFO_IS_RECOVERED" : "STR_UFO_IS_NOT_RECOVERED";
            else if (target == "STR_ALIEN_BASE")
                outcome.title = outcome.success ? "STR_ALIEN_BASE_DESTROYED" : "STR_ALIEN_BASE_STILL_INTACT";
            else
                outcome.title = outcome.success ? "STR_ALIENS_DEFEATED" : "STR_TERROR_CONTINUES";
        }
        return outcome;
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
