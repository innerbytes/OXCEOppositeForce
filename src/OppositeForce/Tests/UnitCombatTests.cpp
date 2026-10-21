#include "Battlescape/TileEngine.h"
#include "Engine/Options.h"
#include "Fixtures.h"
#include "Mod/RuleDamageType.h"
#include "OppositeForce/Battlescape/UnitCombatRules.h"
#include "Savegame/BattleItem.h"
#include "Savegame/BattleUnit.h"
#include "Savegame/BattleUnitStatistics.h"
#include "Savegame/SavedBattleGame.h"
#include "Savegame/SoldierDiary.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

namespace
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
}  // namespace

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

TEST_CASE("Casualty_ExcludedTarget_PreservesAttributionWithoutJournalEntry", "[unit-combat][kills]")
{
    // Arrange
    const auto status = GENERATE(STATUS_DEAD, STATUS_UNCONSCIOUS);
    CombatFixture fixture;
    auto event = fixture.createEvent(status);
    std::vector<BattleUnit *> units{fixture.attacker.get(), fixture.target.get()};

    // Act
    UnitCombatRules::suppressCasualtyRecord(*fixture.target, fixture.attacker.get(), event, units);

    // Assert
    REQUIRE(event.status == STATUS_IGNORE_ME);
    REQUIRE(fixture.target->getMurdererId() == fixture.attacker->getId());
    REQUIRE(fixture.attacker->getStatistics()->kills.empty());
}

TEST_CASE("Casualty_DefaultTarget_PreservesExistingEventAndAttribution", "[unit-combat][kills]")
{
    // Arrange
    const auto flags = GENERATE("{}", "cosmetic: true");
    CombatFixture fixture(flags);
    auto event = fixture.createEvent(STATUS_DEAD);

    // Act
    UnitCombatRules::suppressCasualtyRecord(*fixture.target, fixture.attacker.get(), event, {});

    // Assert
    REQUIRE(event.status == STATUS_DEAD);
    REQUIRE(fixture.target->getMurdererId() == 0);
}

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

TEST_CASE("BattleUnit_ExcludedTarget_PreservesShotStatisticsAndIndependentTraining", "[unit-combat][experience]")
{
    // Arrange
    CombatFixture fixture;
    fixture.attacker->getStatistics()->shotsFiredCounter = 4;
    fixture.attacker->getStatistics()->shotsLandedCounter = 3;
    fixture.attacker->getStatistics()->longDistanceHitCounter = 1;
    auto event = fixture.createEvent(STATUS_DEAD);

    // Act
    UnitCombatRules::suppressCasualtyRecord(*fixture.target, fixture.attacker.get(), event, {});
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
