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

        /// Restores ordinary mission completion and clears the pending scripted result.
        void reset();

        /// Enables or disables script-controlled mission completion.
        void setManualCompletion(bool manual);
        /// Returns whether ordinary objective and enemy-elimination completion is suppressed.
        bool isManualCompletion() const;

        /// Sets or clears the pending scripted mission result.
        void setResult(Result result);
        /// Returns the pending scripted mission result.
        Result getResult() const;

        /// Loads mission completion state from a battle save, using defaults when the fields are absent.
        void load(const YAML::YamlNodeReader &reader);
        /// Saves the manual-completion setting and pending result.
        void save(YAML::YamlNodeWriter writer) const;

    private:
        bool _isManualCompletion = false;
        Result _pendingResult = Result::None;
    };
}  // namespace OpenXcom
