#pragma once

namespace OpenXcom
{
    class BattleUnit;
}  // namespace OpenXcom

namespace OppositeForce::UnitCombatRules
{
    /// Whether damage to this target grants neither casualty records nor attacker experience.
    bool shouldDisableKillRecords(const OpenXcom::BattleUnit *target);

    /// Reject positive damage-generated stun, preserving explicit stun and healing.
    int filterStunDamage(const OpenXcom::BattleUnit &target, int damage);
}  // namespace OppositeForce::UnitCombatRules
