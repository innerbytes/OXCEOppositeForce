#pragma once

#include <vector>

namespace OpenXcom
{
    class BattleUnit;
    struct BattleUnitKills;

    namespace UnitCombatRules
    {
        /// Whether damage to this target grants neither casualty records nor attacker experience.
        bool shouldDisableKillRecords(const BattleUnit *target);

        /// Preserve casualty attribution and mark only the local journal event as ignored.
        void suppressCasualtyRecord(BattleUnit &target, BattleUnit *attacker, BattleUnitKills &event,
                                    const std::vector<BattleUnit *> &units);

        /// Reject positive damage-generated stun, preserving explicit stun and healing.
        int filterStunDamage(const BattleUnit &target, int damage);
    }  // namespace UnitCombatRules
}  // namespace OpenXcom
