#include "UnitCombatRules.h"

#include "../../../Mod/Unit.h"
#include "../../../Savegame/BattleUnit.h"

namespace OppositeForce::UnitCombatRules
{
    using namespace OpenXcom;

    bool shouldDisableKillRecords(const BattleUnit *target)
    {
        return target && target->getUnitRules() && target->getUnitRules()->ofGetDisableKillRecords();
    }

    int filterStunDamage(const BattleUnit &target, int damage)
    {
        const auto *rule = target.getUnitRules();
        return damage > 0 && rule && !rule->ofCanBeStunned() ? 0 : damage;
    }
}  // namespace OppositeForce::UnitCombatRules
