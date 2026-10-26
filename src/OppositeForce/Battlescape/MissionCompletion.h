#pragma once

#include <string>

#include "../../Engine/Yaml.h"

namespace OpenXcom
{
    class AlienDeployment;

    /// Saved manual-ending mode and pending script result.
    class MissionCompletion
    {
    public:
        enum class Result
        {
            None = 0,
            Success = 1,
            Failure = 2
        };

        struct DebriefingOutcome
        {
            bool success;
            std::string title;
            std::string objectiveText;
            int objectiveScore = 0;
        };

        void reset();
        void setManualCompletion(bool manual);

        // TODO - looks like it's used only in tests, remove
        bool isManualCompletion() const;

        void setResult(Result result);

        Result getResult() const;

        bool shouldSuppressAutomaticCompletion() const;

        /// Consume a pending result at a safe turn transition, for any faction.
        Result consumePendingResult(int liveSoldiers, bool preview);
        /// Discard queued requests and retain only an explicitly supplied script result.
        Result prepareFinish(Result scriptResult, bool aborted, bool preview);

        static bool resolveSuccess(Result result, bool nativeSuccess, int survivors);

        // TODO - see if we need to control it from here
        static DebriefingOutcome getDebriefingOutcome(Result result, const AlienDeployment *deployment,
                                                      const std::string &target, int survivors, bool lostCraft);

        // TODO - see if we need to control it from here
        static std::string getCutscene(Result result, const AlienDeployment &deployment, int survivors);

        // TODO - load / save code can be moved to the other class
        void load(const YAML::YamlNodeReader &reader);
        void save(YAML::YamlNodeWriter writer) const;

    private:
        bool _isManualCompletion = false;
        Result _pendingResult = Result::None;
    };
}  // namespace OpenXcom
