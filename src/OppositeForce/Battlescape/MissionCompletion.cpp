#include "MissionCompletion.h"

#include "../../Engine/ScriptBind.h"
#include "../../Mod/AlienDeployment.h"
#include "../../Savegame/SavedBattleGame.h"

namespace OpenXcom
{
    namespace
    {
        void setMissionEndManual(SavedBattleGame *battle, int manual)
        {
            if (battle && !battle->isPreview())
            {
                battle->ofGetMissionCompletion().setManual(manual != 0);
            }
        }

        void completeMission(SavedBattleGame *battle, int success)
        {
            if (battle && !battle->isPreview())
            {
                battle->ofGetMissionCompletion().requestCompletion(success != 0);
            }
        }
    }  // namespace

    void MissionCompletion::setManual(bool manual)
    {
        _manual = manual;
    }

    bool MissionCompletion::isManual() const
    {
        return _manual;
    }

    void MissionCompletion::requestCompletion(bool success)
    {
        if (!hasAppliedResult())
        {
            _pending = success ? Result::Success : Result::Failure;
        }
    }

    bool MissionCompletion::hasPendingResult() const
    {
        return _pending != Result::None;
    }

    bool MissionCompletion::hasAppliedResult() const
    {
        return _applied != Result::None;
    }

    bool MissionCompletion::hasArmedCompletion() const
    {
        return _armed;
    }

    bool MissionCompletion::shouldSuppressAutomaticCompletion(bool preview) const
    {
        return !preview && (_manual || hasPendingResult());
    }

    void MissionCompletion::armCompletion(bool playerTurn, bool preview)
    {
        _armed = playerTurn && !preview && hasPendingResult();
    }

    bool MissionCompletion::tryApplyCompletion(bool playerTurn, int liveSoldiers, bool preview)
    {
        if (!_armed)
        {
            return false;
        }
        _armed = false;
        if (!playerTurn || preview || liveSoldiers <= 0 || !hasPendingResult())
        {
            return false;
        }
        _applied = _pending;
        _pending = Result::None;
        return true;
    }

    void MissionCompletion::prepareFinish(bool aborted)
    {
        _armed = false;
        _pending = Result::None;
        if (aborted)
        {
            _applied = Result::None;
        }
    }

    void MissionCompletion::resetForStage()
    {
        *this = MissionCompletion{};
    }

    bool MissionCompletion::resolveSuccess(bool nativeSuccess, int survivors) const
    {
        return hasAppliedResult() ? _applied == Result::Success && survivors > 0 : nativeSuccess;
    }

    MissionCompletion::DebriefingOutcome MissionCompletion::getDebriefingOutcome(const AlienDeployment *deployment,
                                                                                 const std::string &target,
                                                                                 int survivors, bool lostCraft) const
    {
        DebriefingOutcome outcome{resolveSuccess(false, survivors) && !lostCraft, {}, {}, 0};
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

    std::string MissionCompletion::getCutscene(const AlienDeployment &deployment, int survivors) const
    {
        return resolveSuccess(false, survivors) ? deployment.getWinCutscene() : deployment.getLoseCutscene();
    }

    void MissionCompletion::load(const YAML::YamlNodeReader &reader)
    {
        resetForStage();
        const auto &node = reader["ofMissionCompletion"];
        if (node)
        {
            node.tryRead("manual", _manual);
            int pending = -1, applied = -1;
            node.tryRead("pending", pending);
            node.tryRead("applied", applied);
            if (pending == 0 || pending == 1) _pending = static_cast<Result>(pending);
            if (applied == 0 || applied == 1) _applied = static_cast<Result>(applied);
            if (hasAppliedResult()) _pending = Result::None;
        }
    }

    void MissionCompletion::save(YAML::YamlNodeWriter writer) const
    {
        if (_manual || hasPendingResult() || hasAppliedResult())
        {
            writer.setAsMap();
            auto node = writer["ofMissionCompletion"];
            node.setAsMap();
            node.write("manual", _manual);
            node.write("pending", static_cast<int>(_pending));
            node.write("applied", static_cast<int>(_applied));
        }
    }

    void MissionCompletion::registerScriptBindings(ScriptParserBase *parser)
    {
        Bind<SavedBattleGame> binding{parser, BindBase::ExtensionBinding{}};
        binding.add<&setMissionEndManual>(
            "setMissionEndManual",
            "1: script-owned ending; 0: normal completion. Defeat, timers and retreat remain active.");
        binding.add<&completeMission>(
            "completeMission",
            "Queue success (1) or failure (0) for the next player Next Turn press. Latest request wins.");
    }
}  // namespace OpenXcom
