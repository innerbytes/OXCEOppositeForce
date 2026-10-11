#include "Fixtures.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

TEST_CASE("RuleSoldier_DefaultConstruction_AllowsAvatarSelection", "[avatar][rules]")
{
    // Arrange
    RuleSoldier rule("DEFAULT", 0);

    // Act
    const bool disabled = rule.ofGetDisableAvatarSelection();

    // Assert
    REQUIRE_FALSE(disabled);
}

TEST_CASE("RuleSoldier_OptionalAvatarRestriction_LoadsExpectedValue", "[avatar][rules]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, bool>({
        {"{}", false},
        {"disableAvatarSelection: false", false},
        {"disableAvatarSelection: true", true},
        {"refNode:\n  disableAvatarSelection: true", true},
        {"refNode:\n  disableAvatarSelection: true\ndisableAvatarSelection: false", false},
        {"refNode:\n  disableAvatarSelection: false\ndisableAvatarSelection: true", true},
    }));
    CAPTURE(yaml);
    RulesFixture fixture;
    RuleSoldier rule("OPTIONAL", 0);

    // Act
    fixture.loadRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofGetDisableAvatarSelection() == expected);
}

TEST_CASE("RuleSoldier_LayeredAvatarRestriction_PreservesOrOverridesPreviousValue", "[avatar][rules]")
{
    // Arrange
    const auto [initial, layer, expected] = GENERATE(table<const char *, const char *, bool>({
        {"disableAvatarSelection: true", "costBuy: 100", true},
        {"disableAvatarSelection: false", "costBuy: 100", false},
        {"disableAvatarSelection: true", "disableAvatarSelection: false", false},
        {"disableAvatarSelection: false", "disableAvatarSelection: true", true},
    }));
    RulesFixture fixture;
    auto &rule = fixture.createSoldier("LAYERED", initial);

    // Act
    fixture.loadRule(rule, layer);

    // Assert
    REQUIRE(rule.ofGetDisableAvatarSelection() == expected);
}

TEST_CASE("Soldier_RuleRestrictionChanges_PreservesExistingAppearance", "[avatar]")
{
    // Arrange
    const auto [gender, look, variant] = GENERATE(table<SoldierGender, SoldierLook, int>({
        {GENDER_MALE, LOOK_BLONDE, 0},
        {GENDER_FEMALE, LOOK_AFRICAN, 3},
    }));
    RulesFixture fixture;
    auto &rule = fixture.createSoldier("SOLDIER");
    auto &armor = fixture.createArmor("ARMOR");
    Soldier soldier(&rule, &armor, 0, 0);
    soldier.setGender(gender);
    soldier.setLook(look);
    soldier.setLookVariant(variant);

    // Act
    fixture.loadRule(rule, "disableAvatarSelection: true");

    // Assert
    REQUIRE(soldier.getRules()->ofGetDisableAvatarSelection());
    REQUIRE(soldier.getGender() == gender);
    REQUIRE(soldier.getLook() == look);
    REQUIRE(soldier.getLookVariant() == variant);
}

TEST_CASE("Soldier_ReloadWithAvatarRestriction_PreservesSavedAppearance", "[avatar][save]")
{
    // Arrange
    RulesFixture fixture;
    auto &rule = fixture.createSoldier("SOLDIER", "disableAvatarSelection: true");
    auto &armor = fixture.createArmor("ARMOR");
    Soldier original(&rule, &armor, 0, 0);
    original.setGender(GENDER_FEMALE);
    original.setLook(LOOK_ORIENTAL);
    original.setLookVariant(2);
    YAML::YamlRootNodeWriter snapshot;
    original.save(snapshot, fixture.getShared());
    SavedGame save;
    Soldier reloaded(&rule, &armor, 0, 0);

    // Act
    reloaded.load(snapshot.toReader(), &fixture.getMod(), &save, fixture.getShared());

    // Assert
    REQUIRE(reloaded.getRules()->ofGetDisableAvatarSelection());
    REQUIRE(reloaded.getGender() == GENDER_FEMALE);
    REQUIRE(reloaded.getLook() == LOOK_ORIENTAL);
    REQUIRE(reloaded.getLookVariant() == 2);
}
