#pragma once

#include <string>

#include "../../Engine/Yaml.h"

namespace OpenXcom
{
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

        // TODO - load / save code can be moved to the other class
        void load(const YAML::YamlNodeReader &reader);
        void save(YAML::YamlNodeWriter writer) const;

    private:
        bool _isManualCompletion = false;
        Result _pendingResult = Result::None;
    };
}  // namespace OpenXcom
