#include "MissionCompletion.h"

namespace OppositeForce
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

    void MissionCompletion::reset()
    {
        _isManualCompletion = false;
        _pendingResult = Result::None;
    }

}  // namespace OppositeForce
