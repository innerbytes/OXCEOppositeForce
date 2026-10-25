#pragma once

#include <string>

#include "../../Engine/Yaml.h"

namespace OpenXcom
{
    class AlienDeployment;
    class ScriptParserBase;

    /// Saved script-owned mission result, independent of retreat and recovery policy.
    class MissionCompletion
    {
    public:
        enum class Result
        {
            None = -1,
            Failure = 0,
            Success = 1
        };

        struct DebriefingOutcome
        {
            bool success;
            std::string title;
            std::string objectiveText;
            int objectiveScore = 0;
        };

        void setManual(bool manual);
        bool isManual() const;
        void requestCompletion(bool success);
        bool hasPendingResult() const;
        bool hasAppliedResult() const;
        bool hasArmedCompletion() const;
        bool shouldSuppressAutomaticCompletion(bool preview) const;
        void armCompletion(bool playerTurn, bool preview);
        bool tryApplyCompletion(bool playerTurn, int liveSoldiers, bool preview);
        void prepareFinish(bool aborted);
        void resetForStage();
        bool resolveSuccess(bool nativeSuccess, int survivors) const;
        DebriefingOutcome getDebriefingOutcome(const AlienDeployment *deployment, const std::string &target,
                                               int survivors, bool lostCraft) const;
        std::string getCutscene(const AlienDeployment &deployment, int survivors) const;
        void load(const YAML::YamlNodeReader &reader);
        void save(YAML::YamlNodeWriter writer) const;
        static void registerScriptBindings(ScriptParserBase *parser);

    private:
        bool _manual = false;
        Result _pending = Result::None;
        Result _applied = Result::None;
        bool _armed = false;
    };
}  // namespace OpenXcom
