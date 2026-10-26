#include "Battlescape/TileEngine.h"
#include "CombatFixtures.h"
#include "Engine/Options.h"
#include "Fixtures.h"
#include "Mod/RuleDamageType.h"
#include "OppositeForce/Battlescape/Game/UnitCombatRules.h"
#include "Savegame/BattleItem.h"
#include "Savegame/BattleUnit.h"
#include "Savegame/BattleUnitStatistics.h"
#include "Savegame/SavedBattleGame.h"
#include "Savegame/SoldierDiary.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OppositeForce;
using namespace OppositeForce::Tests;

TEST_CASE("Experience_ExcludedTarget_BlocksManaAndScriptAwards", "[unit-combat][experience][scripts]")
{
    // Arrange
    const bool disabled = GENERATE(false, true);
    CombatFixture fixture(disabled ? "disableKillRecords: true" : "{}",
                          "scripts:\n  awardExperience: |\n    set experience_multipler 300;\n"
                          "    return experience_multipler;\n");
    auto &rule = fixture.rules.createItem("GUN", "battleType: 1\nmanaExperience: 100\n");
    int itemId = 1;
    BattleItem weapon(&rule, &itemId);

    // Act
    const bool hit = fixture.hitTarget(BattleActionAttack{BA_SNAPSHOT, fixture.attacker.get(), &weapon, &weapon});

    // Assert
    REQUIRE(hit);
    REQUIRE(fixture.attacker->_exp.firing == (disabled ? 0 : 3));
    REQUIRE(fixture.attacker->_exp.mana == (disabled ? 0 : 1));
    REQUIRE(fixture.target->getHealth() == 90);
}

TEST_CASE("Experience_ExcludedTarget_BlocksReactionAwards", "[unit-combat][experience]")
{
    // Arrange
    const bool disabled = GENERATE(false, true);
    CombatFixture fixture(disabled ? "disableKillRecords: true" : "{}");
    std::vector<TileEngine::ReactionScore> spotters{{fixture.attacker.get(), nullptr, BA_SNAPSHOT, 1000.0, 0.0, 1}};

    // Act
    const auto *reactor = fixture.engine->getReactor(spotters, fixture.target.get());

    // Assert
    REQUIRE(reactor != nullptr);
    REQUIRE(fixture.attacker->_exp.reactions == (disabled ? 0 : 1));
    REQUIRE(fixture.attacker->hasGainedAnyExperience() == !disabled);
}

TEST_CASE("Damage_StunDisabled_RejectsStunRegardlessOfPainImmunity", "[unit-combat][stun]")
{
    // Arrange
    const bool stunnable = GENERATE(false, true);
    const bool painImmune = GENERATE(false, true);
    const bool bypass = GENERATE(false, true);
    const auto resistance = GENERATE(DT_STUN, DT_SMOKE, DT_HE);
    CombatFixture fixture(stunnable ? "canBeStunned: true" : "canBeStunned: false",
                          painImmune ? "painImmune: true" : "painImmune: false");
    auto damage = fixture.createDamage(bypass, 0.5f);
    damage.ResistType = resistance;

    // Act
    fixture.applyDamage(damage);

    // Assert
    REQUIRE(fixture.target->getStunlevel() == (stunnable && (!painImmune || bypass) ? 20 : 0));
    REQUIRE(fixture.target->getHealth() == 90);
}

TEST_CASE("Damage_ScriptAddsStun_RespectsFinalStunRestriction", "[unit-combat][stun][scripts]")
{
    // Arrange
    const bool stunnable = GENERATE(false, true);
    CombatFixture fixture(stunnable ? "{}" : "canBeStunned: false",
                          "scripts:\n  damageUnit: |\n    set to_stun 50;\n    return;\n");
    const auto damage = fixture.createDamage(true);

    // Act
    fixture.applyDamage(damage);

    // Assert
    REQUIRE(fixture.target->getStunlevel() == (stunnable ? 50 : 0));
    REQUIRE(fixture.target->getHealth() == 100);
}

TEST_CASE("BattleUnit_ExcludedTarget_PreservesShotStatisticsAndIndependentTraining", "[unit-combat][experience]")
{
    // Arrange
    CombatFixture fixture;
    fixture.attacker->getStatistics()->shotsFiredCounter = 4;
    fixture.attacker->getStatistics()->shotsLandedCounter = 3;
    fixture.attacker->getStatistics()->longDistanceHitCounter = 1;
    auto &rule = fixture.rules.createItem("GUN", "battleType: 1\nmaxRange: 30");
    int itemId = 1;
    BattleItem weapon(&rule, &itemId);

    // Act
    fixture.hitTarget(BattleActionAttack{BA_SNAPSHOT, fixture.attacker.get(), &weapon, &weapon});
    fixture.attacker->addThrowingExp();

    // Assert
    REQUIRE(fixture.attacker->getStatistics()->shotsFiredCounter == 4);
    REQUIRE(fixture.attacker->getStatistics()->shotsLandedCounter == 3);
    REQUIRE(fixture.attacker->getStatistics()->longDistanceHitCounter == 1);
    REQUIRE(fixture.attacker->_exp.throwing == 1);
}

TEST_CASE("Experience_DefaultHostileTarget_RetainsNormalHitTraining", "[unit-combat][experience]")
{
    // Arrange
    const bool disabled = GENERATE(false, true);
    CombatFixture fixture(disabled ? "disableKillRecords: true" : "{}");
    fixture.target->_originalFaction = FACTION_HOSTILE;
    fixture.target->convertToFaction(FACTION_HOSTILE);
    auto &rule = fixture.rules.createItem("GUN", "battleType: 1\nmaxRange: 30");
    int itemId = 1;
    BattleItem weapon(&rule, &itemId);

    // Act
    fixture.hitTarget(BattleActionAttack{BA_SNAPSHOT, fixture.attacker.get(), &weapon, &weapon});

    // Assert
    REQUIRE(fixture.attacker->_exp.firing == (disabled ? 0 : 1));
}

TEST_CASE("BattleUnit_ExistingSaveWithCombatFlags_PreservesPriorRecordsAndExplicitStun", "[unit-combat][save]")
{
    // Arrange
    CombatFixture fixture;
    fixture.target->addKillCount();
    fixture.target->_stunlevel = 9;
    fixture.target->getStatistics()->kills.push_back(new BattleUnitKills(fixture.createEvent(STATUS_DEAD)));
    YAML::YamlRootNodeWriter snapshot;
    fixture.target->save(snapshot, fixture.rules.getShared());
    BattleUnit reloaded(&fixture.rules.getMod(), &fixture.targetRule, FACTION_NEUTRAL, BattleUnit::MAX_SOLDIER_ID,
                        nullptr, &fixture.armor, nullptr, 0, nullptr);

    // Act
    reloaded.load(snapshot.toReader(), &fixture.rules.getMod(), fixture.rules.getShared());

    // Assert
    REQUIRE(reloaded._kills == 1);
    REQUIRE(reloaded.getStunlevel() == 9);
    REQUIRE(reloaded.getStatistics()->kills.size() == 1);
    REQUIRE(UnitCombatRules::shouldDisableKillRecords(&reloaded));
    REQUIRE(UnitCombatRules::filterStunDamage(reloaded, 20) == 0);
}

TEST_CASE("BattleUnit_ExcludedTarget_CannotTriggerRookiePromotion", "[unit-combat][promotion]")
{
    // Arrange
    const bool disabled = GENERATE(false, true);
    CombatFixture fixture(disabled ? "disableKillRecords: true" : "{}");
    SavedGame save;
    auto *base = new Base(&fixture.rules.getMod());
    save.getBases()->push_back(base);
    auto *soldier = new Soldier(&fixture.soldierRule, &fixture.armor, 0, 8);
    *soldier->getCurrentStatsEditable() = *fixture.targetRule.getStats();
    base->getSoldiers()->push_back(soldier);
    BattleUnit attacker(&fixture.rules.getMod(), soldier, 0, nullptr);
    fixture.target->_originalFaction = FACTION_HOSTILE;
    fixture.target->convertToFaction(FACTION_HOSTILE);
    auto &rule = fixture.rules.createItem("GUN", "battleType: 1\nmaxRange: 30");
    int itemId = 1;
    BattleItem weapon(&rule, &itemId);
    StatAdjustment growth{};

    // Act
    fixture.hitTarget(BattleActionAttack{BA_SNAPSHOT, &attacker, &weapon, &weapon});
    attacker.postMissionProcedures(&fixture.rules.getMod(), &save, &fixture.battle, growth);

    // Assert
    REQUIRE(soldier->getRank() == (disabled ? RANK_ROOKIE : RANK_SQUADDIE));
    REQUIRE(soldier->getMissions() == 1);
}
