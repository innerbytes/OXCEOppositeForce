#pragma once

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

namespace OpenXcom::OppositeForceTests
{
    class CombatFixture
    {
        bool _previousExperienceMode = Mod::EXTENDED_EXPERIENCE_AWARD_SYSTEM;

        int _previousPersonalLightMode = Options::oxceTogglePersonalLightType;

    public:
        RulesFixture rules;

        Language language;

        Armor &armor;

        Unit &targetRule;

        RuleSoldier &soldierRule;

        Soldier soldier;

        SavedBattleGame battle;

        std::unique_ptr<TileEngine> engine;

        std::unique_ptr<BattleUnit> attacker;

        std::unique_ptr<BattleUnit> target;

        explicit CombatFixture(const std::string &flags = "disableKillRecords: true\ncanBeStunned: false",
                               const std::string &armorYaml = "{}")
            : armor(rules.createArmor("ARMOR")),
              targetRule(rules.createUnit("OBJECT",
                                          "stats: {tu: 50, health: 100, stamina: 100, reactions: 50, "
                                          "firing: 50, strength: 50}\n" +
                                              (flags == "{}" ? "" : flags))),
              soldierRule(rules.createSoldier("SOLDIER")),
              soldier(&soldierRule, &armor, 0, 7),
              battle(&rules.getMod(), &language, false)
        {
            Mod::EXTENDED_EXPERIENCE_AWARD_SYSTEM = true;
            Options::oxceTogglePersonalLightType = 0;
            rules.loadRule(armor, armorYaml);
            rules.createInventoryGround();
            *soldier.getCurrentStatsEditable() = *targetRule.getStats();
            attacker = std::make_unique<BattleUnit>(&rules.getMod(), &soldier, 0, nullptr);
            target = std::make_unique<BattleUnit>(&rules.getMod(), &targetRule, FACTION_NEUTRAL,
                                                  BattleUnit::MAX_SOLDIER_ID, nullptr, &armor, nullptr, 0, nullptr);
            engine = std::make_unique<TileEngine>(&battle, &rules.getMod());
        }

        ~CombatFixture()
        {
            Mod::EXTENDED_EXPERIENCE_AWARD_SYSTEM = _previousExperienceMode;
            Options::oxceTogglePersonalLightType = _previousPersonalLightMode;
        }

        RuleDamageType createDamage(bool bypassPain, float healthDamage = 0.0f)
        {
            RuleDamageType damage;
            damage.ResistType = DT_STUN;
            damage.IgnorePainImmunity = bypassPain;
            damage.ArmorEffectiveness = 0.0f;
            damage.ToHealth = healthDamage;
            damage.ToStun = 1.0f;
            damage.ToWound = 0.0f;
            damage.RandomStun = false;
            return damage;
        }

        BattleUnitKills createEvent(UnitStatus status)
        {
            BattleUnitKills event;
            event.status = status;
            event.id = target->getId();
            event.faction = target->getOriginalFaction();
            return event;
        }

        void applyDamage(const RuleDamageType &damage)
        {
            target->damage(Position{}, 20, &damage, &battle, BattleActionAttack{BA_SNAPSHOT, attacker.get()});
        }

        bool hitTarget(BattleActionAttack attack, bool ranged = true)
        {
            const auto damage = createDamage(false, 0.5f);
            return engine->hitUnit(attack, target.get(), Position{}, 20, &damage, ranged);
        }
    };
}  // namespace OpenXcom::OppositeForceTests
