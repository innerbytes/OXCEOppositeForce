#include <type_traits>

#include "Battlescape/BattlescapeGame.h"
#include "Battlescape/BattlescapeState.h"
#include "Engine/Options.h"
#include "Fixtures.h"
#include "Mod/ModScript.h"
#include "OppositeForce/Battlescape/MissionCompletion.h"
#include "Savegame/BattleUnit.h"
#include "Savegame/SavedBattleGame.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

namespace
{
    MissionCompletion::Result createScriptResult(bool success)
    {
        MissionCompletion completion;
        completion.setResult(success ? MissionCompletion::Result::Success : MissionCompletion::Result::Failure);
        return completion.getResult();
    }

    void runCreationScript(RulesFixture &rules, SavedBattleGame *battle, const std::string &code)
    {
        auto &armor = rules.createArmor("SCRIPT_ARMOR");
        rules.loadRule(armor, "scripts:\n  createUnit: |\n    " + code + " return;\n");
        ModScript::scriptCallback<ModScript::CreateUnit>(&armor, static_cast<BattleUnit *>(nullptr), battle, 0);
    }
}  // namespace

TEST_CASE("BattlescapeState_NativeFinishBattle_PreservesUpstreamSignature", "[mission-completion][integration]")
{
    // Arrange
    using NativeFinishSignature = void (BattlescapeState::*)(bool, int);

    // Act
    constexpr bool matchesNativeSignature =
        std::is_same_v<decltype(&BattlescapeState::finishBattle), NativeFinishSignature>;

    // Assert
    STATIC_REQUIRE(matchesNativeSignature);
}

TEST_CASE("MissionCompletion_DefaultState_PreservesNativeResults", "[mission-completion]")
{
    // Arrange
    MissionCompletion completion;

    // Act
    const auto result = completion.getResult();

    // Assert
    REQUIRE_FALSE(completion.isManualCompletion());
    REQUIRE_FALSE(completion.shouldSuppressAutomaticCompletion());
    REQUIRE(completion.getResult() == MissionCompletion::Result::None);
    REQUIRE(result == MissionCompletion::Result::None);
    REQUIRE(MissionCompletion::resolveSuccess(true, result, 1));
    REQUIRE_FALSE(MissionCompletion::resolveSuccess(false, result, 1));
}

TEST_CASE("MissionCompletion_ManualModeToggle_RestoresNativeChecksWithoutClearingRequest", "[mission-completion]")
{
    // Arrange
    const bool pending = GENERATE(false, true);
    MissionCompletion completion;
    completion.setManualCompletion(true);
    if (pending) completion.setResult(MissionCompletion::Result::Success);
    REQUIRE(completion.shouldSuppressAutomaticCompletion());

    // Act
    completion.setManualCompletion(false);

    // Assert
    REQUIRE_FALSE(completion.isManualCompletion());
    REQUIRE(completion.getResult() == (pending ? MissionCompletion::Result::Success : MissionCompletion::Result::None));
    REQUIRE(completion.shouldSuppressAutomaticCompletion() == pending);
}

TEST_CASE("MissionCompletion_SetResult_ReplacesOrClearsPendingResultInEitherMode", "[mission-completion]")
{
    // Arrange
    const bool manual = GENERATE(false, true);
    const auto requestedResult = GENERATE(MissionCompletion::Result::None, MissionCompletion::Result::Success,
                                          MissionCompletion::Result::Failure);
    MissionCompletion completion;
    completion.setManualCompletion(manual);
    completion.setResult(MissionCompletion::Result::Success);

    // Act
    completion.setResult(requestedResult);
    const auto result = completion.getResult();
    completion.setResult(MissionCompletion::Result::None);

    // Assert
    REQUIRE(result == requestedResult);
    REQUIRE(completion.getResult() == MissionCompletion::Result::None);
    REQUIRE(MissionCompletion::resolveSuccess(false, result, 1) ==
            (requestedResult == MissionCompletion::Result::Success));
}

TEST_CASE("MissionCompletion_SquadDefeat_OverridesScriptVictory", "[mission-completion]")
{
    // Arrange
    MissionCompletion completion;
    completion.setResult(MissionCompletion::Result::Success);

    // Act
    const bool success = MissionCompletion::resolveSuccess(true, completion.getResult(), 0);

    // Assert
    REQUIRE_FALSE(success);
}

TEST_CASE("MissionCompletion_Reset_RestoresDefaultState", "[mission-completion]")
{
    // Arrange
    const bool success = GENERATE(false, true);
    MissionCompletion completion;
    completion.setManualCompletion(true);
    completion.setResult(success ? MissionCompletion::Result::Success : MissionCompletion::Result::Failure);

    // Act
    completion.reset();

    // Assert
    REQUIRE_FALSE(completion.isManualCompletion());
    REQUIRE(completion.getResult() == MissionCompletion::Result::None);
    REQUIRE_FALSE(completion.shouldSuppressAutomaticCompletion());
}

TEST_CASE("MissionCompletion_ExplicitResult_OverridesNativeObjectiveAndPresentation", "[mission-completion]")
{
    // Arrange
    const bool success = GENERATE(false, true);
    const std::string target = GENERATE("STR_BASE", "STR_UFO", "STR_ALIEN_BASE", "STR_MISSION_SITE");
    RulesFixture rules;
    auto &deployment =
        rules.createDeployment("SCRIPTED",
                               "nextStage: NEXT\nescapeType: 1\nvipSurvivalPercentage: 100\n"
                               "objectiveComplete: [OBJECTIVE_WON, 500]\nobjectiveFailed: [OBJECTIVE_LOST, -200]\n"
                               "missionCompleteText: MISSION_WON\nmissionFailedText: MISSION_LOST\n");
    const auto result = createScriptResult(success);

    // Act
    const auto outcome = MissionCompletion::getDebriefingOutcome(result, &deployment, target, 2, false);

    // Assert
    REQUIRE(MissionCompletion::resolveSuccess(!success, result, 2) == success);
    REQUIRE(outcome.success == success);
    REQUIRE(outcome.title == (success ? "MISSION_WON" : "MISSION_LOST"));
    REQUIRE(outcome.objectiveText == (success ? "OBJECTIVE_WON" : "OBJECTIVE_LOST"));
    REQUIRE(outcome.objectiveScore == (success ? 500 : -200));
}

TEST_CASE("MissionCompletion_LaterRequest_PreservesExplicitFinalResult", "[mission-completion]")
{
    // Arrange
    const bool success = GENERATE(false, true);
    MissionCompletion completion;
    completion.setResult(success ? MissionCompletion::Result::Success : MissionCompletion::Result::Failure);
    const auto result = completion.getResult();
    completion.setResult(MissionCompletion::Result::None);

    // Act
    completion.setResult(success ? MissionCompletion::Result::Failure : MissionCompletion::Result::Success);
    const auto outcome = MissionCompletion::getDebriefingOutcome(result, nullptr, "STR_UFO", 1, false);

    // Assert
    REQUIRE(outcome.success == success);
    REQUIRE(outcome.title == (success ? "STR_UFO_IS_RECOVERED" : "STR_UFO_IS_NOT_RECOVERED"));
}

TEST_CASE("MissionCompletion_SquadOrCraftLost_OverridesScriptVictory", "[mission-completion]")
{
    // Arrange
    const bool lostCraft = GENERATE(false, true);
    const auto result = createScriptResult(true);

    // Act
    const auto outcome =
        MissionCompletion::getDebriefingOutcome(result, nullptr, "STR_UFO", lostCraft ? 1 : 0, lostCraft);

    // Assert
    REQUIRE_FALSE(outcome.success);
    REQUIRE(outcome.title == (lostCraft ? "STR_CRAFT_IS_LOST" : "STR_UFO_IS_NOT_RECOVERED"));
}

TEST_CASE("MissionCompletion_SaveRoundTrip_PreservesPendingResult", "[mission-completion][save]")
{
    // Arrange
    const bool success = GENERATE(false, true);
    MissionCompletion original;
    original.setManualCompletion(true);
    original.setResult(success ? MissionCompletion::Result::Success : MissionCompletion::Result::Failure);
    YAML::YamlRootNodeWriter snapshot;
    original.save(snapshot);
    MissionCompletion reloaded;

    // Act
    reloaded.load(snapshot.toReader());
    const auto result = reloaded.getResult();

    // Assert
    REQUIRE(reloaded.isManualCompletion());
    REQUIRE(result == (success ? MissionCompletion::Result::Success : MissionCompletion::Result::Failure));
    REQUIRE(reloaded.getResult() == result);
    REQUIRE(snapshot.toReader()["ofMissionCompletion"]["isManualCompletion"].readVal<bool>());
    REQUIRE(snapshot.toReader()["ofMissionCompletion"]["pendingResult"].readVal<int>() == (success ? 1 : 2));
    REQUIRE_FALSE(snapshot.toReader()["isManualCompletion"]);
    REQUIRE_FALSE(snapshot.toReader()["pendingResult"]);
}

TEST_CASE("MissionCompletion_SavedKeys_LoadsManualModeAndPendingResult", "[mission-completion][save]")
{
    // Arrange
    const bool manual = GENERATE(false, true);
    const auto [pendingResult, expectedResult] = GENERATE(table<int, MissionCompletion::Result>({
        {-1, MissionCompletion::Result::None},
        {0, MissionCompletion::Result::None},
        {1, MissionCompletion::Result::Success},
        {2, MissionCompletion::Result::Failure},
    }));
    YAML::YamlRootNodeReader snapshot(
        YAML::YamlString("ofMissionCompletion: {isManualCompletion: " + std::string(manual ? "true" : "false") +
                         ", pendingResult: " + std::to_string(pendingResult) + "}"),
        "mission completion save");
    MissionCompletion completion;
    completion.setManualCompletion(!manual);
    completion.setResult(MissionCompletion::Result::Success);

    // Act
    completion.load(snapshot);
    const auto result = completion.getResult();

    // Assert
    REQUIRE(completion.isManualCompletion() == manual);
    REQUIRE(result == expectedResult);
}

TEST_CASE("MissionCompletion_OldSaveWithoutFields_ResetsToNativeBehavior", "[mission-completion][save]")
{
    // Arrange
    MissionCompletion completion;
    completion.setManualCompletion(true);
    completion.setResult(MissionCompletion::Result::Success);
    YAML::YamlRootNodeReader oldSave(YAML::YamlString("turn: 1"), "old battle save");

    // Act
    completion.load(oldSave);

    // Assert
    REQUIRE_FALSE(completion.isManualCompletion());
    REQUIRE(completion.getResult() == MissionCompletion::Result::None);
    REQUIRE_FALSE(completion.shouldSuppressAutomaticCompletion());
}

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

TEST_CASE("BattleGame_SaveRoundTrip_PreservesManualCompletionAndNativeTimer", "[mission-completion][save]")
{
    // Arrange
    const bool success = GENERATE(false, true);
    const bool manual = GENERATE(false, true);
    RulesFixture rules;
    Language language;
    SavedGame campaign;
    SavedBattleGame original(&rules.getMod(), &language, false);
    original.initMap(1, 1, 1);
    original.setTurnLimit(20);
    original.setChronoTrigger(FORCE_LOSE);
    original.ofGetMissionCompletion().setManualCompletion(manual);
    original.ofGetMissionCompletion().setResult(success ? MissionCompletion::Result::Success
                                                        : MissionCompletion::Result::Failure);
    YAML::YamlRootNodeWriter snapshot;
    original.save(snapshot);
    SavedBattleGame reloaded(&rules.getMod(), &language, false);

    // Act
    YAML::YamlRootNodeReader serialized(snapshot.emit(), "battle save round trip");
    reloaded.load(serialized, &rules.getMod(), &campaign);
    const auto result = reloaded.ofGetMissionCompletion().getResult();

    // Assert
    REQUIRE(reloaded.ofGetMissionCompletion().isManualCompletion() == manual);
    REQUIRE(result == (success ? MissionCompletion::Result::Success : MissionCompletion::Result::Failure));
    REQUIRE(reloaded.getTurnLimit() == 20);
    REQUIRE(reloaded.getChronoTrigger() == FORCE_LOSE);
    REQUIRE_FALSE(reloaded.isAborted());
}

TEST_CASE("BattleGame_ManualOrPendingCompletion_DoesNotAutoEndOrLatchEnemyElimination", "[mission-completion][battle]")
{
    // Arrange
    const bool autoEnd = GENERATE(false, true);
    const bool manual = GENERATE(false, true);
    const bool destroyedObjective = GENERATE(false, true);
    const bool eliminationAlreadyLatched = GENERATE(false, true);
    const bool previousAutoEnd = Options::battleAutoEnd;
    RulesFixture rules;
    Language language;
    auto &armor = rules.createArmor("ARMOR");
    auto &soldierRule = rules.createSoldier("SOLDIER");
    Soldier soldier(&soldierRule, &armor, 0, 7);
    soldier.getCurrentStatsEditable()->health = 40;
    rules.createInventoryGround();
    SavedBattleGame battle(&rules.getMod(), &language, false);
    SavedGame campaign;
    YAML::YamlRootNodeReader initialState(
        YAML::YamlString(
            "width: 1\nlength: 1\nheight: 1\nselectedUnit: -1\nobjectiveType: " + std::to_string(MUST_DESTROY) +
            "\nobjectivesNeeded: 1\nobjectivesDestroyed: " + (destroyedObjective ? "1" : "0")),
        "completion battle");
    battle.load(initialState, &rules.getMod(), &campaign);
    battle.initUtilities(&rules.getMod());
    // Construction with an empty non-player phase avoids unrelated UI refreshes.
    battle._side = FACTION_HOSTILE;
    BattlescapeGame engine(&battle, nullptr);
    battle._side = FACTION_PLAYER;
    engine._allEnemiesNeutralized = eliminationAlreadyLatched;
    auto *unit = new BattleUnit(&rules.getMod(), &soldier, 0, nullptr);
    battle.getUnits()->push_back(unit);
    battle.ofGetMissionCompletion().setManualCompletion(manual);
    if (!manual) battle.ofGetMissionCompletion().setResult(MissionCompletion::Result::Success);
    Options::battleAutoEnd = autoEnd;

    // Act
    engine.autoEndBattle();
    Options::battleAutoEnd = previousAutoEnd;

    // Assert
    REQUIRE_FALSE(engine.areAllEnemiesNeutralized());
    REQUIRE(engine.getStates().empty());
    REQUIRE(battle.getTurn() == 0);
    REQUIRE_FALSE(battle.isAborted());
}
