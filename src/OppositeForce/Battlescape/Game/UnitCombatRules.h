#pragma once

namespace OpenXcom
{
    class BattleUnit;
}  // namespace OpenXcom

namespace OpenXcom::UnitCombatRules
{
    /// Whether damage to this target grants neither casualty records nor attacker experience.
    bool shouldDisableKillRecords(const BattleUnit *target);

    /// Reject positive damage-generated stun, preserving explicit stun and healing.
    int filterStunDamage(const BattleUnit &target, int damage);
}  // namespace OpenXcom::UnitCombatRules
