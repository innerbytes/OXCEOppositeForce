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
using namespace OpenXcom::OppositeForceTests;

TEST_CASE("BattleUnit_NoUnitRule_PreservesSoldierCombatBehavior", "[unit-combat]")
{
    // Arrange
    CombatFixture fixture;

    // Act
    const bool disabled = UnitCombatRules::shouldDisableKillRecords(fixture.attacker.get());
    const int stun = UnitCombatRules::filterStunDamage(*fixture.attacker, 20);

    // Assert
    REQUIRE_FALSE(disabled);
    REQUIRE(stun == 20);
}
