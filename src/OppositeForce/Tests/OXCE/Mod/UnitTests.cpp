#include "Battlescape/TileEngine.h"
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

TEST_CASE("Unit_DefaultConstruction_PreservesCombatBehavior", "[unit-combat][rules]")
{
    // Arrange
    Unit rule("DEFAULT");

    // Act
    const bool disabled = rule.ofGetDisableKillRecords();
    const bool stunnable = rule.ofCanBeStunned();

    // Assert
    REQUIRE_FALSE(disabled);
    REQUIRE(stunnable);
}

TEST_CASE("Unit_OptionalCombatFlags_LoadInheritedAndExplicitValues", "[unit-combat][rules]")
{
    // Arrange
    const auto [yaml, disabled, stunnable] = GENERATE(table<const char *, bool, bool>({
        {"{}", false, true},
        {"disableKillRecords: false\ncanBeStunned: true", false, true},
        {"disableKillRecords: true", true, true},
        {"canBeStunned: false", false, false},
        {"refNode: {disableKillRecords: true, canBeStunned: false}", true, false},
        {"refNode: {disableKillRecords: true, canBeStunned: false}\ndisableKillRecords: false\ncanBeStunned: true",
         false, true},
        {"refNode: {disableKillRecords: false, canBeStunned: true}\ndisableKillRecords: true\ncanBeStunned: false",
         true, false},
    }));
    RulesFixture fixture;
    Unit rule("OPTIONAL");

    // Act
    fixture.loadRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofGetDisableKillRecords() == disabled);
    REQUIRE(rule.ofCanBeStunned() == stunnable);
}

TEST_CASE("Unit_LayeredCombatFlags_PreserveOrOverridePreviousValues", "[unit-combat][rules]")
{
    // Arrange
    const auto [layer, disabled, stunnable] = GENERATE(table<const char *, bool, bool>({
        {"value: 10", true, false},
        {"disableKillRecords: false", false, false},
        {"canBeStunned: true", true, true},
    }));
    RulesFixture fixture;
    auto &rule = fixture.createUnit("LAYERED", "disableKillRecords: true\ncanBeStunned: false");

    // Act
    fixture.loadRule(rule, layer);

    // Assert
    REQUIRE(rule.ofGetDisableKillRecords() == disabled);
    REQUIRE(rule.ofCanBeStunned() == stunnable);
}
