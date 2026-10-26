#include "Battlescape/BattlescapeGame.h"
#include "Battlescape/DebriefingState.h"
#include "Fixtures.h"
#include "OppositeForce/Battlescape/Game/MissionCompletionResult.h"
#include "Savegame/SavedBattleGame.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OppositeForce;
using namespace OppositeForce::Tests;

TEST_CASE("MissionCompletionResult_NoScriptResult_PreservesNativeOutcome", "[mission-completion]")
{
    // Arrange
    const bool nativeSuccess = GENERATE(false, true);
    const int survivorsCount = GENERATE(0, 1);

    // Act
    const bool success =
        MissionCompletionResult::resolveSuccess(nativeSuccess, MissionCompletion::Result::None, survivorsCount);

    // Assert
    REQUIRE(success == nativeSuccess);
}

TEST_CASE("MissionCompletionResult_SquadDefeat_OverridesScriptVictory", "[mission-completion]")
{
    // Arrange
    MissionCompletion completion;
    completion.setResult(MissionCompletion::Result::Success);

    // Act
    const bool success = MissionCompletionResult::resolveSuccess(true, completion.getResult(), 0);

    // Assert
    REQUIRE_FALSE(success);
}

TEST_CASE("MissionCompletionResult_ExplicitResult_OverridesNativeObjective", "[mission-completion]")
{
    // Arrange
    const bool success = GENERATE(false, true);
    const auto result = success ? MissionCompletion::Result::Success : MissionCompletion::Result::Failure;

    // Act
    const bool resolvedSuccess = MissionCompletionResult::resolveSuccess(!success, result, 2);

    // Assert
    REQUIRE(resolvedSuccess == success);
}

TEST_CASE("MissionCompletionResult_DefaultDebriefingTitle_MatchesOutcomeAndTarget", "[mission-completion]")
{
    // Arrange
    const bool success = GENERATE(false, true);
    const auto [target, winTitle, loseTitle] = GENERATE(table<std::string, std::string, std::string>({
        {"STR_BASE", "STR_BASE_IS_SAVED", "STR_BASE_IS_LOST"},
        {"STR_UFO", "STR_UFO_IS_RECOVERED", "STR_UFO_IS_NOT_RECOVERED"},
        {"STR_ALIEN_BASE", "STR_ALIEN_BASE_DESTROYED", "STR_ALIEN_BASE_STILL_INTACT"},
        {"STR_MISSION_SITE", "STR_ALIENS_DEFEATED", "STR_TERROR_CONTINUES"},
    }));

    // Act
    const auto title = MissionCompletionResult::getDefaultDebriefingTitle(success, target, false);

    // Assert
    REQUIRE(title == (success ? winTitle : loseTitle));
}

TEST_CASE("MissionCompletionResult_InvalidDebriefingOutcome_ReturnsEmptyTitle", "[mission-completion]")
{
    // Arrange
    const auto [success, target, lostCraft] = GENERATE(table<bool, std::string, bool>({
        {true, "STR_UFO", true},
        {true, "UNKNOWN_TARGET", false},
        {false, "UNKNOWN_TARGET", false},
        {true, "", false},
        {false, "", false},
    }));

    // Act
    const auto title = MissionCompletionResult::getDefaultDebriefingTitle(success, target, lostCraft);

    // Assert
    REQUIRE(title.empty());
}

TEST_CASE("MissionCompletionResult_ClearObjectiveAwards_ResetsBothAwardsAndPreservesOtherStats", "[mission-completion]")
{
    // Arrange
    DebriefingStat completed("OBJECTIVE_WON", false);
    completed.qty = 3;
    completed.score = 500;
    DebriefingStat failed("OBJECTIVE_LOST", false);
    failed.qty = 1;
    failed.score = -200;
    DebriefingStat recovered("RECOVERED_ITEM", true);
    recovered.qty = 4;
    recovered.score = 40;
    const std::vector<DebriefingStat *> stats{&completed, &failed, &recovered};

    // Act
    MissionCompletionResult::clearObjectiveAwards(stats, "OBJECTIVE_WON", "OBJECTIVE_LOST");

    // Assert
    REQUIRE(stats.size() == 3);
    REQUIRE(completed.item == "OBJECTIVE_WON");
    REQUIRE(completed.qty == 0);
    REQUIRE(completed.score == 0);
    REQUIRE_FALSE(completed.recovery);
    REQUIRE(failed.item == "OBJECTIVE_LOST");
    REQUIRE(failed.qty == 0);
    REQUIRE(failed.score == 0);
    REQUIRE_FALSE(failed.recovery);
    REQUIRE(recovered.item == "RECOVERED_ITEM");
    REQUIRE(recovered.qty == 4);
    REQUIRE(recovered.score == 40);
    REQUIRE(recovered.recovery);
}

TEST_CASE("MissionCompletionResult_EmptyOrMissingObjectiveLabels_PreservesStats", "[mission-completion]")
{
    // Arrange
    const std::string label = GENERATE("", "MISSING_OBJECTIVE");
    DebriefingStat unnamed("", false);
    unnamed.qty = 1;
    unnamed.score = 50;
    DebriefingStat other("OTHER_STAT", false);
    other.qty = 2;
    other.score = 100;
    const std::vector<DebriefingStat *> stats{&unnamed, &other};

    // Act
    MissionCompletionResult::clearObjectiveAwards(stats, label, label);

    // Assert
    REQUIRE(unnamed.qty == 1);
    REQUIRE(unnamed.score == 50);
    REQUIRE(other.qty == 2);
    REQUIRE(other.score == 100);
}

TEST_CASE("MissionCompletionResult_OneConfiguredObjective_ClearsMatchingAward", "[mission-completion]")
{
    // Arrange
    const bool success = GENERATE(false, true);
    DebriefingStat objective("OBJECTIVE", false);
    objective.qty = 1;
    objective.score = success ? 500 : -200;
    const std::vector<DebriefingStat *> stats{&objective};

    // Act
    MissionCompletionResult::clearObjectiveAwards(stats, success ? "OBJECTIVE" : "", success ? "" : "OBJECTIVE");

    // Assert
    REQUIRE(objective.qty == 0);
    REQUIRE(objective.score == 0);
}

TEST_CASE("MissionCompletionResult_SharedObjectiveLabel_ClearsAwardIdempotently", "[mission-completion]")
{
    // Arrange
    DebriefingStat objective("OBJECTIVE", false);
    objective.qty = 2;
    objective.score = 300;
    const std::vector<DebriefingStat *> stats{&objective};

    // Act
    MissionCompletionResult::clearObjectiveAwards(stats, "OBJECTIVE", "OBJECTIVE");
    MissionCompletionResult::clearObjectiveAwards(stats, "OBJECTIVE", "OBJECTIVE");

    // Assert
    REQUIRE(objective.item == "OBJECTIVE");
    REQUIRE(objective.qty == 0);
    REQUIRE(objective.score == 0);
}

TEST_CASE("MissionCompletionResult_LaterRequest_PreservesExplicitFinalResult", "[mission-completion]")
{
    // Arrange
    const bool success = GENERATE(false, true);
    MissionCompletion completion;
    completion.setResult(success ? MissionCompletion::Result::Success : MissionCompletion::Result::Failure);
    const auto result = completion.getResult();
    completion.setResult(MissionCompletion::Result::None);

    // Act
    completion.setResult(success ? MissionCompletion::Result::Failure : MissionCompletion::Result::Success);
    const bool resolvedSuccess = MissionCompletionResult::resolveSuccess(!success, result, 1);

    // Assert
    REQUIRE(resolvedSuccess == success);
}

TEST_CASE("MissionCompletionResult_LostCraft_OverridesDefaultTargetTitle", "[mission-completion]")
{
    // Arrange
    const std::string target = GENERATE("STR_BASE", "STR_UFO", "STR_ALIEN_BASE", "STR_MISSION_SITE");

    // Act
    const auto title = MissionCompletionResult::getDefaultDebriefingTitle(false, target, true);

    // Assert
    REQUIRE(title == "STR_CRAFT_IS_LOST");
}

TEST_CASE("MissionCompletionResult_EndTurnEnemyElimination_RespectsManualModeAndVIPRequirements",
          "[mission-completion]")
{
    // Arrange
    const bool manual = GENERATE(false, true);
    const auto result = GENERATE(MissionCompletion::Result::Success, MissionCompletion::Result::Failure);
    const auto [liveAliens, latched, vipPercentage, escapeType, nativeEnding] =
        GENERATE(table<int, bool, int, EscapeType, bool>({
            {1, false, 0, ESCAPE_NONE, false},
            {0, false, 0, ESCAPE_NONE, true},
            {1, true, 0, ESCAPE_NONE, true},
            {0, true, 0, ESCAPE_NONE, true},
            {0, false, 100, ESCAPE_EXIT, false},
            {1, true, 100, ESCAPE_EXIT, false},
            {0, false, 100, ESCAPE_NONE, true},
            {0, false, 0, ESCAPE_EXIT, true},
        }));
    RulesFixture rules;
    Language language;
    SavedBattleGame battle(&rules.getMod(), &language, false);
    battle.setVIPSurvivalPercentage(vipPercentage);
    battle.setVIPEscapeType(escapeType);
    battle.ofGetMissionCompletion().setManualCompletion(manual);
    battle.ofGetMissionCompletion().setResult(result);
    BattlescapeTally tally{};
    tally.liveSoldiers = 1;
    tally.liveAliens = liveAliens;
    const int originalTurn = battle.getTurn();

    // Act
    const bool finish = MissionCompletionResult::shouldScriptedMissionResultApplyOnEndTurn(battle, tally, latched);

    // Assert
    REQUIRE(finish == (manual || !nativeEnding));
    REQUIRE(battle.ofGetMissionCompletion().getResult() == result);
    REQUIRE(battle.getTurn() == originalTurn);
}

TEST_CASE("MissionCompletionResult_EndTurnDestroyObjective_RequiresNativeObjectiveCompletion", "[mission-completion]")
{
    // Arrange
    const bool manual = GENERATE(false, true);
    const bool destroyed = GENERATE(false, true);
    const bool latched = GENERATE(false, true);
    const auto result = GENERATE(MissionCompletion::Result::Success, MissionCompletion::Result::Failure);
    RulesFixture rules;
    Language language;
    SavedGame campaign;
    SavedBattleGame battle(&rules.getMod(), &language, false);
    YAML::YamlRootNodeReader snapshot(
        YAML::YamlString(
            "width: 1\nlength: 1\nheight: 1\nselectedUnit: -1\nobjectiveType: " + std::to_string(MUST_DESTROY) +
            "\nobjectivesNeeded: 1\nobjectivesDestroyed: " + (destroyed ? "1" : "0")),
        "objective completion");
    battle.load(snapshot, &rules.getMod(), &campaign);
    battle.ofGetMissionCompletion().setManualCompletion(manual);
    battle.ofGetMissionCompletion().setResult(result);
    BattlescapeTally tally{};
    tally.liveSoldiers = 1;
    tally.liveAliens = 0;

    // Act
    const bool finish = MissionCompletionResult::shouldScriptedMissionResultApplyOnEndTurn(battle, tally, latched);

    // Assert
    REQUIRE(finish == (manual || !destroyed));
    REQUIRE(battle.ofGetMissionCompletion().getResult() == result);
}

TEST_CASE("MissionCompletionResult_EndTurnTimer_OnlyExpiredEnabledTimerBlocksScript", "[mission-completion]")
{
    // Arrange
    const bool manual = GENERATE(false, true);
    const auto result = GENERATE(MissionCompletion::Result::Success, MissionCompletion::Result::Failure);
    const auto [turn, limit, expectedFinish] = GENERATE(table<int, int, bool>({
        {5, 0, true},
        {4, 5, true},
        {5, 5, true},
        {6, 5, false},
    }));
    RulesFixture rules;
    Language language;
    SavedGame campaign;
    SavedBattleGame battle(&rules.getMod(), &language, false);
    YAML::YamlRootNodeReader snapshot(
        YAML::YamlString("width: 1\nlength: 1\nheight: 1\nselectedUnit: -1\nturn: " + std::to_string(turn)),
        "completion timer");
    battle.load(snapshot, &rules.getMod(), &campaign);
    battle.setTurnLimit(limit);
    battle.ofGetMissionCompletion().setManualCompletion(manual);
    battle.ofGetMissionCompletion().setResult(result);
    BattlescapeTally tally{};
    tally.liveSoldiers = 1;
    tally.liveAliens = 1;

    // Act
    const bool finish = MissionCompletionResult::shouldScriptedMissionResultApplyOnEndTurn(battle, tally, false);

    // Assert
    REQUIRE(finish == expectedFinish);
    REQUIRE(battle.getTurn() == turn);
    REQUIRE(battle.ofGetMissionCompletion().getResult() == result);
}

TEST_CASE("MissionCompletionResult_EndTurnSquadDefeat_BlocksScriptInEitherMode", "[mission-completion]")
{
    // Arrange
    const bool manual = GENERATE(false, true);
    const auto result = GENERATE(MissionCompletion::Result::Success, MissionCompletion::Result::Failure);
    RulesFixture rules;
    Language language;
    SavedBattleGame battle(&rules.getMod(), &language, false);
    battle.ofGetMissionCompletion().setManualCompletion(manual);
    battle.ofGetMissionCompletion().setResult(result);
    BattlescapeTally tally{};
    tally.liveSoldiers = 0;
    tally.liveAliens = 1;

    // Act
    const bool finish = MissionCompletionResult::shouldScriptedMissionResultApplyOnEndTurn(battle, tally, false);

    // Assert
    REQUIRE_FALSE(finish);
    REQUIRE(battle.ofGetMissionCompletion().getResult() == result);
}
