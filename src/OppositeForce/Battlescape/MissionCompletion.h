#pragma once

#include <string>
#include <vector>

#include "../../Engine/Yaml.h"

namespace OpenXcom
{
    struct DebriefingStat;

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

        void reset();
        void setManualCompletion(bool manual);

        // TODO - looks like it's used only in tests, remove
        bool isManualCompletion() const;

        void setResult(Result result);

        Result getResult() const;

        bool shouldSuppressAutomaticCompletion() const;

        static bool resolveSuccess(bool isNativeSuccess, Result scriptResult, int survivorsCount);

        static std::string getDefaultDebriefingTitle(bool success, const std::string &target, bool lostCraft);

        static void clearObjectiveAwards(const std::vector<DebriefingStat *> &stats,
                                         const std::string &objectiveCompleteText,
                                         const std::string &objectiveFailedText);

        void load(const YAML::YamlNodeReader &reader);
        void save(YAML::YamlNodeWriter writer) const;

    private:
        bool _isManualCompletion = false;
        Result _pendingResult = Result::None;
    };
}  // namespace OpenXcom
