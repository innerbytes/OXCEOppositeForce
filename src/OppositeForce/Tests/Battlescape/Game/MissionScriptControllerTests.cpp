#include <type_traits>

#include "Battlescape/BattlescapeGame.h"
#include "Battlescape/BattlescapeState.h"
#include "Engine/Options.h"
#include "Fixtures.h"
#include "MissionScriptFixtures.h"
#include "Mod/ModScript.h"
#include "OppositeForce/Battlescape/Game/MissionCompletion.h"
#include "Savegame/BattleUnit.h"
#include "Savegame/SavedBattleGame.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OppositeForce;
using namespace OppositeForce::Tests;

TEST_CASE("BattleGame_MissionCompletionCommands_UpdateSavedBattleState", "[mission-completion][scripts]")
{
    // Arrange
    const bool success = GENERATE(false, true);
    RulesFixture rules;
    Language language;
    SavedBattleGame battle(&rules.getMod(), &language, false);

    // Act
    runCreationScript(rules, &battle,
                      "battle_game.setMissionEndManual 1; battle_game.completeMission 0; "
                      "battle_game.setMissionEndManual 0; battle_game.completeMission " +
                          std::string(success ? "1" : "0") + ";");

    // Assert
    auto &completion = battle.ofGetMissionCompletion();
    REQUIRE_FALSE(completion.isManualCompletion());
    REQUIRE(completion.getResult() ==
            (success ? MissionCompletion::Result::Success : MissionCompletion::Result::Failure));
    REQUIRE_FALSE(battle.isAborted());
    REQUIRE(battle.getTurn() == 0);
}

TEST_CASE("BattleGame_PreviewOrNullScriptTarget_IgnoresCompletionCommands", "[mission-completion][scripts]")
{
    // Arrange
    const bool nullTarget = GENERATE(false, true);
    RulesFixture rules;
    Language language;
    SavedBattleGame battle(&rules.getMod(), &language, true);

    // Act
    runCreationScript(rules, nullTarget ? nullptr : &battle,
                      "battle_game.setMissionEndManual 1; battle_game.completeMission 1;");

    // Assert
    REQUIRE_FALSE(battle.ofGetMissionCompletion().isManualCompletion());
    REQUIRE(battle.ofGetMissionCompletion().getResult() == MissionCompletion::Result::None);
}
