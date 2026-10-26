#include "MissionScriptController.h"

#include "../../Engine/ScriptBind.h"
#include "../../Savegame/SavedBattleGame.h"

namespace OpenXcom::MissionScriptController
{
    static void setMissionEndManual(SavedBattleGame *battle, int manual)
    {
        if (battle && !battle->isPreview())
        {
            battle->ofGetMissionCompletion().setManualCompletion(manual != 0);
        }
    }

    static void completeMission(SavedBattleGame *battle, int success)
    {
        if (battle && !battle->isPreview())
        {
            const auto result = success != 0 ? MissionCompletion::Result::Success : MissionCompletion::Result::Failure;
            battle->ofGetMissionCompletion().setResult(result);
        }
    }

    void registerScriptBindings(ScriptParserBase *parser)
    {
        Bind<SavedBattleGame> binding{parser, BindBase::ExtensionBinding{}};
        binding.add<&setMissionEndManual>(
            "setMissionEndManual",
            "1: script-owned ending; 0: normal completion. Defeat, timers and retreat remain active.");
        binding.add<&completeMission>(
            "completeMission", "Queue success (1) or failure (0) for the next turn transition. Latest request wins.");
    }
}  // namespace OpenXcom::MissionScriptController
