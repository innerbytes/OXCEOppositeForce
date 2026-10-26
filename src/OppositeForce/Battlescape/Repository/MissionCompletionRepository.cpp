#include "MissionCompletionRepository.h"

#include "../Game/MissionCompletion.h"

namespace OppositeForce::MissionCompletionRepository
{
    using namespace OpenXcom;

    void load(MissionCompletion &completion, const YAML::YamlNodeReader &reader)
    {
        completion.reset();
        const auto &node = reader["ofMissionCompletion"];
        if (node)
        {
            bool manual = false;
            node.tryRead("isManualCompletion", manual);
            completion.setManualCompletion(manual);
            int pendingResult = 0;
            node.tryRead("pendingResult", pendingResult);
            completion.setResult(pendingResult >= 0 ? static_cast<MissionCompletion::Result>(pendingResult)
                                                    : MissionCompletion::Result::None);
        }
    }

    void save(const MissionCompletion &completion, YAML::YamlNodeWriter writer)
    {
        writer.setAsMap();
        auto node = writer["ofMissionCompletion"];
        node.setAsMap();
        node.write("isManualCompletion", completion.isManualCompletion());
        node.write("pendingResult", static_cast<int>(completion.getResult()));
    }
}  // namespace OppositeForce::MissionCompletionRepository
