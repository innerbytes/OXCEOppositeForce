#pragma once

#include <type_traits>

#include "Battlescape/BattlescapeGame.h"
#include "Battlescape/BattlescapeState.h"
#include "Engine/Options.h"
#include "Fixtures.h"
#include "Mod/ModScript.h"
#include "OppositeForce/Battlescape/Game/MissionCompletion.h"
#include "Savegame/BattleUnit.h"
#include "Savegame/SavedBattleGame.h"

namespace OppositeForce::Tests
{
    using namespace OpenXcom;

    static void runCreationScript(RulesFixture &rules, SavedBattleGame *battle, const std::string &code)
    {
        auto &armor = rules.createArmor("SCRIPT_ARMOR");
        rules.loadRule(armor, "scripts:\n  createUnit: |\n    " + code + " return;\n");
        ModScript::scriptCallback<ModScript::CreateUnit>(&armor, static_cast<BattleUnit *>(nullptr), battle, 0);
    }
}  // namespace OppositeForce::Tests
