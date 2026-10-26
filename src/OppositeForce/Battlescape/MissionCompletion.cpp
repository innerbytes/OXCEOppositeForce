#include "MissionCompletion.h"

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
