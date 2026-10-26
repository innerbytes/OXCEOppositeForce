#pragma once

#include <string>

#include "../../Engine/Yaml.h"

namespace OpenXcom
{
    class AlienDeployment;

    /// Saved script-owned mission result, independent of retreat and recovery policy.
    class MissionCompletion
    {
    public:
        // TODO - is None result is needed? Should None be 0 , Success 1 and Failure -1?
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

        void reset();
        void setManual(bool manual);

        // TODO - looks like it's used only in tests, remove
        bool isManual() const;

        void requestCompletion(bool success);

        // TODO - looks like it's used only in tests, remove
        bool hasPendingResult() const;

        bool hasAppliedResult() const;

        bool hasArmedCompletion() const;

        bool shouldSuppressAutomaticCompletion(bool preview) const;

        // TODO - see how it's different from requestCompletion()
        void armCompletion(bool playerTurn, bool preview);

        bool tryApplyCompletion(bool playerTurn, int liveSoldiers, bool preview);

        // TODO - see what it's doing
        void prepareFinish(bool aborted);

        bool resolveSuccess(bool nativeSuccess, int survivors) const;

        // TODO - see if we need to control it from here
        DebriefingOutcome getDebriefingOutcome(const AlienDeployment *deployment, const std::string &target,
                                               int survivors, bool lostCraft) const;

        // TODO - see if we need to control it from here
        std::string getCutscene(const AlienDeployment &deployment, int survivors) const;

        // TODO - load / save code can be moved to the other class
        void load(const YAML::YamlNodeReader &reader);
        void save(YAML::YamlNodeWriter writer) const;

    private:
        // TODO - move to separate storage and have good names

        // TODO rename to _isManualCompletion;
        bool _manual = false;

        Result _pending = Result::None;
        Result _applied = Result::None;

        // TODO - see how this dirty flag us used
        bool _armed = false;
    };
}  // namespace OpenXcom
