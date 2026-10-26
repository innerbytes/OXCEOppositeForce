#pragma once

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

        void load(const YAML::YamlNodeReader &reader);
        void save(YAML::YamlNodeWriter writer) const;

    private:
        bool _isManualCompletion = false;
        Result _pendingResult = Result::None;
    };
}  // namespace OpenXcom
