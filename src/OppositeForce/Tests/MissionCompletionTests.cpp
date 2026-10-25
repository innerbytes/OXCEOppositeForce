#include "Battlescape/BattlescapeGame.h"
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
    MissionCompletion createAppliedCompletion(bool success)
    {
        MissionCompletion completion;
        completion.requestCompletion(success);
        completion.armCompletion(true, false);
        completion.tryApplyCompletion(true, 1, false);
        return completion;
    }

    void runCreationScript(RulesFixture &rules, SavedBattleGame *battle, const std::string &code)
    {
        auto &armor = rules.createArmor("SCRIPT_ARMOR");
        rules.loadRule(armor, "scripts:\n  createUnit: |\n    " + code + " return;\n");
        ModScript::scriptCallback<ModScript::CreateUnit>(&armor, static_cast<BattleUnit *>(nullptr), battle, 0);
    }
}  // namespace

TEST_CASE("MissionCompletion_DefaultState_PreservesNativeResults", "[mission-completion]")
{
    // Arrange
    MissionCompletion completion;

    // Act
    completion.armCompletion(true, false);
    const bool applied = completion.tryApplyCompletion(true, 1, false);

    // Assert
    REQUIRE_FALSE(completion.isManual());
    REQUIRE_FALSE(completion.shouldSuppressAutomaticCompletion(false));
    REQUIRE_FALSE(completion.hasPendingResult());
    REQUIRE_FALSE(completion.hasAppliedResult());
    REQUIRE_FALSE(applied);
    REQUIRE(completion.resolveSuccess(true, 1));
    REQUIRE_FALSE(completion.resolveSuccess(false, 1));
}

TEST_CASE("MissionCompletion_ManualModeToggle_RestoresNativeChecksWithoutClearingRequest", "[mission-completion]")
{
    // Arrange
    const bool pending = GENERATE(false, true);
    MissionCompletion completion;
    completion.setManual(true);
    if (pending) completion.requestCompletion(true);
    REQUIRE(completion.shouldSuppressAutomaticCompletion(false));

    // Act
    completion.setManual(false);

    // Assert
    REQUIRE_FALSE(completion.isManual());
    REQUIRE(completion.hasPendingResult() == pending);
    REQUIRE(completion.shouldSuppressAutomaticCompletion(false) == pending);
}

TEST_CASE("MissionCompletion_RepeatedRequests_UsesLatestResultInEitherMode", "[mission-completion]")
{
    // Arrange
    const bool manual = GENERATE(false, true);
    const bool success = GENERATE(false, true);
    MissionCompletion completion;
    completion.setManual(manual);
    completion.requestCompletion(!success);
    completion.armCompletion(true, false);

    // Act
    completion.requestCompletion(success);
    const bool applied = completion.tryApplyCompletion(true, 1, false);

    // Assert
    REQUIRE(applied);
    REQUIRE_FALSE(completion.hasPendingResult());
    REQUIRE(completion.hasAppliedResult());
    REQUIRE(completion.resolveSuccess(!success, 1) == success);
    REQUIRE_FALSE(completion.tryApplyCompletion(true, 1, false));
}

TEST_CASE("MissionCompletion_UnarmedOrEnemyTransition_DoesNotApplyPendingRequest", "[mission-completion]")
{
    // Arrange
    const bool playerPress = GENERATE(false, true);
    MissionCompletion completion;
    completion.requestCompletion(true);
    completion.armCompletion(playerPress, false);

    // Act
    const bool applied = completion.tryApplyCompletion(false, 1, false);

    // Assert
    REQUIRE_FALSE(applied);
    REQUIRE(completion.hasPendingResult());
    REQUIRE_FALSE(completion.hasAppliedResult());
    REQUIRE_FALSE(completion.tryApplyCompletion(true, 1, false));
}

TEST_CASE("MissionCompletion_RequestAfterPress_WaitsForAnotherPress", "[mission-completion]")
{
    // Arrange
    MissionCompletion completion;
    completion.armCompletion(true, false);

    // Act
    completion.requestCompletion(true);
    const bool applied = completion.tryApplyCompletion(true, 1, false);

    // Assert
    REQUIRE_FALSE(applied);
    REQUIRE(completion.hasPendingResult());
    completion.armCompletion(true, false);
    REQUIRE(completion.tryApplyCompletion(true, 1, false));
}

TEST_CASE("MissionCompletion_SquadDefeat_PreventsApplyingScriptVictory", "[mission-completion]")
{
    // Arrange
    MissionCompletion completion;
    completion.requestCompletion(true);
    completion.armCompletion(true, false);

    // Act
    const bool applied = completion.tryApplyCompletion(true, 0, false);
    completion.prepareFinish(false);

    // Assert
    REQUIRE_FALSE(applied);
    REQUIRE_FALSE(completion.hasPendingResult());
    REQUIRE_FALSE(completion.hasAppliedResult());
    REQUIRE_FALSE(completion.resolveSuccess(false, 0));
}

TEST_CASE("MissionCompletion_NativeTimerOrRetreat_DiscardsQueuedResult", "[mission-completion]")
{
    // Arrange
    const bool retreat = GENERATE(false, true);
    MissionCompletion completion;
    completion.setManual(true);
    completion.requestCompletion(false);
    completion.armCompletion(true, false);

    // Act
    completion.prepareFinish(retreat);

    // Assert
    REQUIRE_FALSE(completion.hasPendingResult());
    REQUIRE_FALSE(completion.hasAppliedResult());
    REQUIRE(completion.resolveSuccess(true, 1));
    REQUIRE_FALSE(completion.tryApplyCompletion(true, 1, false));
}

TEST_CASE("MissionCompletion_NativeStageTransition_ResetsAllCompletionState", "[mission-completion]")
{
    // Arrange
    const bool applied = GENERATE(false, true);
    auto completion = applied ? createAppliedCompletion(true) : MissionCompletion{};
    completion.setManual(true);
    if (!applied) completion.requestCompletion(false);

    // Act
    completion.resetForStage();

    // Assert
    REQUIRE_FALSE(completion.isManual());
    REQUIRE_FALSE(completion.hasPendingResult());
    REQUIRE_FALSE(completion.hasAppliedResult());
    REQUIRE_FALSE(completion.shouldSuppressAutomaticCompletion(false));
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
                               "missionCompleteText: MISSION_WON\nmissionFailedText: MISSION_LOST\n"
                               "winCutscene: WIN\nloseCutscene: LOSE\nabortCutscene: ABORT\n");
    auto completion = createAppliedCompletion(success);

    // Act
    const auto outcome = completion.getDebriefingOutcome(&deployment, target, 2, false);
    const auto cutscene = completion.getCutscene(deployment, 2);

    // Assert
    REQUIRE(completion.resolveSuccess(!success, 2) == success);
    REQUIRE(outcome.success == success);
    REQUIRE(outcome.title == (success ? "MISSION_WON" : "MISSION_LOST"));
    REQUIRE(outcome.objectiveText == (success ? "OBJECTIVE_WON" : "OBJECTIVE_LOST"));
    REQUIRE(outcome.objectiveScore == (success ? 500 : -200));
    REQUIRE(cutscene == (success ? "WIN" : "LOSE"));
}

TEST_CASE("MissionCompletion_SquadOrCraftLost_OverridesAppliedVictory", "[mission-completion]")
{
    // Arrange
    const bool lostCraft = GENERATE(false, true);
    auto completion = createAppliedCompletion(true);

    // Act
    const auto outcome = completion.getDebriefingOutcome(nullptr, "STR_UFO", lostCraft ? 1 : 0, lostCraft);

    // Assert
    REQUIRE_FALSE(outcome.success);
    REQUIRE(outcome.title == (lostCraft ? "STR_CRAFT_IS_LOST" : "STR_UFO_IS_NOT_RECOVERED"));
}

TEST_CASE("MissionCompletion_SaveRoundTrip_PreservesResultButRequiresNewPress", "[mission-completion][save]")
{
    // Arrange
    const bool success = GENERATE(false, true);
    const bool applied = GENERATE(false, true);
    auto original = applied ? createAppliedCompletion(success) : MissionCompletion{};
    original.setManual(true);
    if (!applied)
    {
        original.requestCompletion(success);
        original.armCompletion(true, false);
    }
    YAML::YamlRootNodeWriter snapshot;
    original.save(snapshot);
    MissionCompletion reloaded;

    // Act
    reloaded.load(snapshot.toReader());
    const bool completedWithoutPress = reloaded.tryApplyCompletion(true, 2, false);

    // Assert
    REQUIRE(reloaded.isManual());
    REQUIRE_FALSE(completedWithoutPress);
    REQUIRE(reloaded.hasAppliedResult() == applied);
    REQUIRE(reloaded.hasPendingResult() == !applied);
    if (!applied)
    {
        reloaded.armCompletion(true, false);
        REQUIRE(reloaded.tryApplyCompletion(true, 2, false));
    }
    REQUIRE(reloaded.resolveSuccess(!success, 2) == success);
}

TEST_CASE("MissionCompletion_OldSaveWithoutFields_ResetsToNativeBehavior", "[mission-completion][save]")
{
    // Arrange
    auto completion = createAppliedCompletion(true);
    completion.setManual(true);
    YAML::YamlRootNodeReader oldSave(YAML::YamlString("turn: 1"), "old battle save");

    // Act
    completion.load(oldSave);

    // Assert
    REQUIRE_FALSE(completion.isManual());
    REQUIRE_FALSE(completion.hasAppliedResult());
    REQUIRE_FALSE(completion.hasPendingResult());
    REQUIRE_FALSE(completion.shouldSuppressAutomaticCompletion(false));
}

TEST_CASE("BattleGame_MissionCompletionCommands_UpdateSavedBattleState", "[mission-completion][scripts]")
{
    // Arrange
    RulesFixture rules;
    Language language;
    SavedBattleGame battle(&rules.getMod(), &language, false);

    // Act
    runCreationScript(rules, &battle,
                      "battle_game.setMissionEndManual 1; battle_game.completeMission 0; "
                      "battle_game.setMissionEndManual 0; battle_game.completeMission 1;");

    // Assert
    auto &completion = battle.ofGetMissionCompletion();
    REQUIRE_FALSE(completion.isManual());
    REQUIRE(completion.hasPendingResult());
    completion.armCompletion(true, false);
    REQUIRE(completion.tryApplyCompletion(true, 1, false));
    REQUIRE(completion.resolveSuccess(false, 1));
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
    REQUIRE_FALSE(battle.ofGetMissionCompletion().isManual());
    REQUIRE_FALSE(battle.ofGetMissionCompletion().hasPendingResult());
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
    original.ofGetMissionCompletion().setManual(manual);
    original.ofGetMissionCompletion().requestCompletion(success);
    original.ofGetMissionCompletion().armCompletion(true, false);
    YAML::YamlRootNodeWriter snapshot;
    original.save(snapshot);
    SavedBattleGame reloaded(&rules.getMod(), &language, false);

    // Act
    YAML::YamlRootNodeReader serialized(snapshot.emit(), "battle save round trip");
    reloaded.load(serialized, &rules.getMod(), &campaign);

    // Assert
    auto &completion = reloaded.ofGetMissionCompletion();
    REQUIRE(completion.isManual() == manual);
    REQUIRE(completion.hasPendingResult());
    REQUIRE_FALSE(completion.tryApplyCompletion(true, 1, false));
    completion.armCompletion(true, false);
    REQUIRE(completion.tryApplyCompletion(true, 1, false));
    REQUIRE(completion.resolveSuccess(!success, 1) == success);
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
    battle.ofGetMissionCompletion().setManual(manual);
    if (!manual) battle.ofGetMissionCompletion().requestCompletion(true);
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
