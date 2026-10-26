#pragma once

namespace OppositeForce
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

    private:
        bool _isManualCompletion = false;

        Result _pendingResult = Result::None;
    };
}  // namespace OppositeForce
