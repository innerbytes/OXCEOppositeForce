#include <type_traits>

#include "Battlescape/BattlescapeGame.h"
#include "Battlescape/BattlescapeState.h"
#include "Engine/Options.h"
#include "Fixtures.h"
#include "Mod/ModScript.h"
#include "OppositeForce/Battlescape/Game/MissionCompletion.h"
#include "Savegame/BattleUnit.h"
#include "Savegame/SavedBattleGame.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

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

TEST_CASE("BattleGame_ManualCompletion_DoesNotAutoEndOrLatchEnemyElimination", "[mission-completion][battle]")
{
    // Arrange
    const bool autoEnd = GENERATE(false, true);
    const auto pendingResult = GENERATE(MissionCompletion::Result::None, MissionCompletion::Result::Success,
                                        MissionCompletion::Result::Failure);
    const bool destroyObjective = GENERATE(false, true);
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
        YAML::YamlString("width: 1\nlength: 1\nheight: 1\nselectedUnit: -1\nobjectiveType: " +
                         std::to_string(destroyObjective ? MUST_DESTROY : 0) +
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
    battle.ofGetMissionCompletion().setManualCompletion(true);
    battle.ofGetMissionCompletion().setResult(pendingResult);
    Options::battleAutoEnd = autoEnd;

    // Act
    engine.autoEndBattle();
    Options::battleAutoEnd = previousAutoEnd;

    // Assert
    REQUIRE(battle.ofGetMissionCompletion().getResult() == pendingResult);
    REQUIRE_FALSE(engine.areAllEnemiesNeutralized());
    REQUIRE(engine.getStates().empty());
    REQUIRE(battle.getTurn() == 0);
    REQUIRE_FALSE(battle.isAborted());
}

TEST_CASE("BattleGame_PendingResult_PreservesNativeLatchAndCompletionRequirements", "[mission-completion][battle]")
{
    // Arrange
    const auto [autoEnd, destroyObjective, destroyedObjective, escortVIP] = GENERATE(table<bool, bool, bool, bool>({
        {false, false, false, false},
        {false, true, true, false},
        {true, true, false, false},
        {true, false, false, true},
    }));
    const auto pendingResult = GENERATE(MissionCompletion::Result::None, MissionCompletion::Result::Success,
                                        MissionCompletion::Result::Failure);
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
        YAML::YamlString("width: 1\nlength: 1\nheight: 1\nselectedUnit: -1\nobjectiveType: " +
                         std::to_string(destroyObjective ? MUST_DESTROY : 0) +
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
    battle.setVIPSurvivalPercentage(escortVIP ? 100 : 0);
    battle.setVIPEscapeType(escortVIP ? ESCAPE_EXIT : ESCAPE_NONE);
    battle.ofGetMissionCompletion().setManualCompletion(false);
    battle.ofGetMissionCompletion().setResult(pendingResult);
    Options::battleAutoEnd = autoEnd;

    // Act
    engine.autoEndBattle();
    Options::battleAutoEnd = previousAutoEnd;

    // Assert
    REQUIRE(battle.ofGetMissionCompletion().getResult() == pendingResult);
    REQUIRE(engine.areAllEnemiesNeutralized() == eliminationAlreadyLatched);
    REQUIRE(engine.getStates().empty());
    REQUIRE(battle.getTurn() == 0);
    REQUIRE_FALSE(battle.isAborted());
}
