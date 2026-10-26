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

TEST_CASE("Damage_StunDisabled_PreservesForcedKnockoutAndStunCosts", "[unit-combat][stun]")
{
    // Arrange
    CombatFixture fixture;
    RuleItemUseCost cost;
    cost.Stun = 5;

    // Act
    fixture.target->spendCost(cost);
    const int costStun = fixture.target->getStunlevel();
    fixture.target->knockOut(nullptr);

    // Assert
    REQUIRE(costStun == 5);
    REQUIRE(fixture.target->getStunlevel() == fixture.target->getHealth());
}

TEST_CASE("Damage_StunDisabled_PreservesExistingStunAndAllowsHealing", "[unit-combat][stun]")
{
    // Arrange
    CombatFixture fixture;
    fixture.target->_stunlevel = 9;
    auto damage = fixture.createDamage(true);

    // Act
    fixture.applyDamage(damage);
    const int afterDamage = fixture.target->getStunlevel();
    damage.ToStun = -1.0f;
    fixture.applyDamage(damage);

    // Assert
    REQUIRE(afterDamage == 9);
    REQUIRE(fixture.target->getStunlevel() == 0);
}
