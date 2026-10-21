#include "UnitCombatRules.h"

#include "../../Mod/Unit.h"
#include "../../Savegame/BattleUnit.h"
#include "../../Savegame/BattleUnitStatistics.h"

namespace OpenXcom::UnitCombatRules
{
    bool shouldDisableKillRecords(const BattleUnit *target)
    {
        return target && target->getUnitRules() && target->getUnitRules()->ofGetDisableKillRecords();
    }

    void suppressCasualtyRecord(BattleUnit &target, BattleUnit *attacker, BattleUnitKills &event,
                                const std::vector<BattleUnit *> &units)
    {
        if (!shouldDisableKillRecords(&target) || !attacker ||
            (event.status != STATUS_DEAD && event.status != STATUS_UNCONSCIOUS))
        {
            return;
        }

        // Keep the original attribution, including credit assigned to a mind controller,
        // without adding the casualty to either unit's statistics.
        if (attacker->getFaction() == FACTION_PLAYER && attacker->getOriginalFaction() != FACTION_PLAYER)
        {
            for (auto *unit : units)
            {
                if (unit->getId() == attacker->getMindControllerId() && unit->getGeoscapeSoldier())
                {
                    target.setMurdererId(unit->getId());
                    break;
                }
            }
        }
        else if (!attacker->getStatistics()->duplicateEntry(event.status, target.getId()))
        {
            target.setMurdererId(attacker->getId());
        }
        event.status = STATUS_IGNORE_ME;
    }

    int filterStunDamage(const BattleUnit &target, int damage)
    {
        const auto *rule = target.getUnitRules();
        return damage > 0 && rule && !rule->ofCanBeStunned() ? 0 : damage;
    }
}  // namespace OpenXcom::UnitCombatRules
