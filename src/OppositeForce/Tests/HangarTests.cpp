#include "Fixtures.h"
#include "OppositeForce/Mod/HangarRules.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

TEST_CASE("HangarCompatibility_BoundedMatchingScenarios_MatchesExhaustiveAssignmentOracle", "[hangars][matching]")
{
    // Arrange
    const int seed = GENERATE(range(0, 64));
    CAPTURE(seed);
    const auto scenario = createMatchingScenario(seed);
    const int expected = calculateExhaustiveFreeSlots(scenario);
    BaseFixture fixture;
    auto &candidate = addMatchingScenario(fixture, scenario);

    // Act
    const int free = HangarCompatibility::getFreeHangarsCount(fixture.getBase(), &candidate);

    // Assert
    REQUIRE(free == expected);
}

TEST_CASE("RuleBaseFacility_OptionalHangarType_LoadsEffectiveTypeAndCapacity", "[hangars][rules]")
{
    // Arrange
    const auto [yaml, expectedType, capacity] = GENERATE(table<const char *, const char *, int>({
        {"crafts: 2", "", 2},
        {"crafts: 1\nhangarType: A", "A", 1},
        {"refNode:\n  crafts: 1\n  hangarType: A", "A", 1},
        {"refNode:\n  crafts: 1\n  hangarType: A\nhangarType: B", "B", 1},
    }));
    CAPTURE(yaml);
    RulesFixture fixture;
    RuleBaseFacility rule("HANGAR", 0);

    // Act
    fixture.loadRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofGetHangarCompatibilityRule().hangarType == expectedType);
    REQUIRE(rule.getCrafts() == capacity);
}

TEST_CASE("RuleBaseFacility_LayeredHangarType_PreservesOrOverridesPreviousType", "[hangars][rules]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, const char *>({
        {"radarRange: 100", "A"},
        {"hangarType: B", "B"},
    }));
    RulesFixture fixture;
    auto &rule = fixture.createFacility("HANGAR", "crafts: 1\nhangarType: A");

    // Act
    fixture.loadRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofGetHangarCompatibilityRule().hangarType == expected);
}

TEST_CASE("RuleBaseFacility_EmptyHangarType_ThrowsValidationException", "[hangars][rules]")
{
    // Arrange
    RulesFixture fixture;
    RuleBaseFacility rule("EMPTY", 0);

    // Act
    const auto load = [&] { fixture.loadRule(rule, "crafts: 1\nhangarType: ''"); };

    // Assert
    REQUIRE_THROWS_AS(load(), Exception);
}

TEST_CASE("HangarRules_TypedFacilityWithSingleCapacity_PassesValidation", "[hangars][rules]")
{
    // Arrange
    RulesFixture fixture;
    auto &rule = fixture.createFacility("VALID", "refNode:\n  crafts: 1\nhangarType: A");

    // Act
    const auto validate = [&] {
        HangarRules::validateFacility(rule.ofGetHangarCompatibilityRule(), rule.getType(), rule.getCrafts());
    };

    // Assert
    REQUIRE_NOTHROW(validate());
}

TEST_CASE("RuleBaseFacility_TypedFacilityWithInvalidEffectiveCapacity_ThrowsDuringAfterLoad", "[hangars][rules]")
{
    // Arrange
    const auto yaml = GENERATE("crafts: 0\nhangarType: A", "crafts: 2\nhangarType: A",
                               "refNode:\n  crafts: 1\n  hangarType: A\ncrafts: 3");
    RulesFixture fixture;
    auto &rule = fixture.createFacility("INVALID", yaml);

    // Act
    const auto validate = [&] { rule.afterLoad(&fixture.getMod()); };

    // Assert
    REQUIRE_THROWS_AS(validate(), Exception);
}

TEST_CASE("HangarRules_UntypedFacilityWithAnyCapacity_PassesCompatibilityValidation", "[hangars][rules]")
{
    // Arrange
    const int capacity = GENERATE(0, 1, 3);
    HangarRules::Facility rule;

    // Act
    const auto validate = [&] { HangarRules::validateFacility(rule, "UNTYPED", capacity); };

    // Assert
    REQUIRE_NOTHROW(validate());
}

TEST_CASE("RuleCraft_OptionalAllowedHangarTypes_LoadsEffectiveRestrictions", "[hangars][rules]")
{
    // Arrange
    const auto [yaml, specified, expected] = GENERATE(table<const char *, bool, std::vector<std::string>>({
        {"{}", false, {}},
        {"allowedHangarTypes: [A, B]", true, {"A", "B"}},
        {"allowedHangarTypes: [A, B, A]", true, {"A", "B"}},
        {"refNode:\n  allowedHangarTypes: [A, B]", true, {"A", "B"}},
        {"refNode:\n  allowedHangarTypes: [A, B]\nallowedHangarTypes: [C]", true, {"C"}},
    }));
    CAPTURE(yaml);
    RulesFixture fixture;
    RuleCraft rule("CRAFT", 0);

    // Act
    fixture.loadRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofGetHangarCompatibilityRule().allowedHangarTypesSpecified == specified);
    REQUIRE(rule.ofGetHangarCompatibilityRule().allowedHangarTypes == expected);
}

TEST_CASE("RuleCraft_LayeredAllowedHangarTypes_PreservesOrReplacesPreviousList", "[hangars][rules]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, std::vector<std::string>>({
        {"costBuy: 100", {"A", "B"}},
        {"allowedHangarTypes: [C, C]", {"C"}},
    }));
    RulesFixture fixture;
    auto &rule = fixture.createCraft("CRAFT", "allowedHangarTypes: [A, B]");

    // Act
    fixture.loadRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofGetHangarCompatibilityRule().allowedHangarTypesSpecified);
    REQUIRE(rule.ofGetHangarCompatibilityRule().allowedHangarTypes == expected);
}

TEST_CASE("RuleCraft_InvalidAllowedHangarTypes_ThrowsValidationException", "[hangars][rules]")
{
    // Arrange
    const auto yaml =
        GENERATE("allowedHangarTypes: A", "allowedHangarTypes: []", "allowedHangarTypes: ['']",
                 "allowedHangarTypes: [A, '']", "refNode:\n  allowedHangarTypes: [A]\nallowedHangarTypes: []");
    CAPTURE(yaml);
    RulesFixture fixture;
    RuleCraft rule("INVALID", 0);

    // Act
    const auto load = [&] { fixture.loadRule(rule, yaml); };

    // Assert
    REQUIRE_THROWS_AS(load(), Exception);
}

TEST_CASE("HangarRules_CraftAndFacilityPair_AppliesExactMatchingOrUnrestrictedFallback", "[hangars][rules]")
{
    // Arrange
    const auto [facilityYaml, craftYaml, expected] = GENERATE(table<const char *, const char *, bool>({
        {"crafts: 1\nhangarType: A", "allowedHangarTypes: [A]", true},
        {"crafts: 1\nhangarType: A", "allowedHangarTypes: [B]", false},
        {"crafts: 1\nhangarType: A", "allowedHangarTypes: [a]", false},
        {"crafts: 1\nhangarType: ARBITRARY_TYPE", "allowedHangarTypes: [ARBITRARY_TYPE]", true},
        {"crafts: 1", "allowedHangarTypes: [A]", true},
        {"crafts: 1\nhangarType: A", "{}", true},
        {"crafts: 1", "{}", true},
    }));
    RulesFixture fixture;
    auto &facility = fixture.createFacility("HANGAR", facilityYaml);
    auto &craft = fixture.createCraft("CRAFT", craftYaml);

    // Act
    const bool accepts =
        HangarRules::accepts(facility.ofGetHangarCompatibilityRule(), craft.ofGetHangarCompatibilityRule());

    // Assert
    REQUIRE(accepts == expected);
}

TEST_CASE("HangarCompatibility_UnannotatedRules_UsesOrdinaryCapacityWithoutOptingIn", "[hangars][matching]")
{
    // Arrange
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("HANGAR", "crafts: 3"));
    auto &craft = fixture.rules.createCraft("CRAFT");
    fixture.addCraft(craft);

    // Act
    const int free = HangarCompatibility::getFreeHangarsCount(fixture.getBase(), &craft);
    const bool enabled = HangarCompatibility::isEnabledForBase(fixture.getBase());

    // Assert
    REQUIRE(free == 2);
    REQUIRE_FALSE(enabled);
    REQUIRE_FALSE(HangarCompatibility::shouldApplyForCraft(fixture.getBase(), &craft));
    REQUIRE_FALSE(HangarCompatibility::calculateDisplayAssignment(fixture.getBase()).hangarCompatibilityEnabled);
}

TEST_CASE("HangarCompatibility_NullCandidate_ReturnsZeroAndDoesNotApplyRestriction", "[hangars][matching]")
{
    // Arrange
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("HANGAR", "crafts: 1\nhangarType: A"));

    // Act
    const int free = HangarCompatibility::getFreeHangarsCount(fixture.getBase(), nullptr);
    const bool applies = HangarCompatibility::shouldApplyForCraft(fixture.getBase(), nullptr);
    const bool lacks = HangarCompatibility::lacksCompatibleSlot(fixture.getBase(), nullptr);

    // Assert
    REQUIRE(free == 0);
    REQUIRE_FALSE(applies);
    REQUIRE_FALSE(lacks);
}

TEST_CASE("HangarCompatibility_RestrictionOnEitherSide_EnablesCandidateCompatibility", "[hangars][matching]")
{
    // Arrange
    const auto [facilityYaml, craftYaml, baseEnabled] = GENERATE(table<const char *, const char *, bool>({
        {"crafts: 1\nhangarType: A", "{}", true},
        {"crafts: 1", "allowedHangarTypes: [A]", false},
    }));
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("HANGAR", facilityYaml));
    auto &candidate = fixture.rules.createCraft("CANDIDATE", craftYaml);

    // Act
    const bool applies = HangarCompatibility::shouldApplyForCraft(fixture.getBase(), &candidate);

    // Assert
    REQUIRE(applies);
    REQUIRE(HangarCompatibility::isEnabledForBase(fixture.getBase()) == baseEnabled);
}

TEST_CASE("HangarCompatibility_UnfinishedHangar_ExcludesUnbuiltCapacity", "[hangars][matching]")
{
    // Arrange
    const auto [buildTime, expected] = GENERATE(table<int, int>({{0, 1}, {10, 0}}));
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("HANGAR", "crafts: 1\nhangarType: A"), buildTime);
    auto &candidate = fixture.rules.createCraft("CANDIDATE", "allowedHangarTypes: [A]");

    // Act
    const int free = HangarCompatibility::getFreeHangarsCount(fixture.getBase(), &candidate);

    // Assert
    REQUIRE(free == expected);
}

TEST_CASE("HangarCompatibility_ResidentOrAirborneCraft_ReservesOneSlot", "[hangars][matching]")
{
    // Arrange
    const auto status = GENERATE("STR_READY", "STR_OUT");
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("HANGAR", "crafts: 2"));
    auto &candidate = fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]");
    fixture.addCraft(candidate, status);

    // Act
    const int free = HangarCompatibility::getFreeHangarsCount(fixture.getBase(), &candidate);

    // Assert
    REQUIRE(free == 1);
}

TEST_CASE("HangarCompatibility_IncomingCraftTransfer_ReservesCompatibleSlot", "[hangars][reservations]")
{
    // Arrange
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("HANGAR", "crafts: 1\nhangarType: A"));
    auto &candidate = fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]");
    fixture.addTransfer(candidate);

    // Act
    const int free = HangarCompatibility::getFreeHangarsCount(fixture.getBase(), &candidate);

    // Assert
    REQUIRE(free == 0);
    REQUIRE(HangarCompatibility::lacksCompatibleSlot(fixture.getBase(), &candidate));
}

TEST_CASE("HangarCompatibility_PendingRuleRows_ReservesRequestedQuantity", "[hangars][reservations]")
{
    // Arrange
    const auto [amount, expected] = GENERATE(table<int, int>({{0, 2}, {1, 1}, {2, 0}, {3, 0}}));
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("HANGAR", "crafts: 2"));
    auto &candidate = fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]");
    const std::vector<TransferRow> pending{createPendingRow(TRANSFER_CRAFT, &candidate, amount)};

    // Act
    const int free = HangarCompatibility::getFreeHangarsCount(fixture.getBase(), &candidate, &pending);

    // Assert
    REQUIRE(free == expected);
}

TEST_CASE("HangarCompatibility_PendingCraftObjectRows_ReservesRequestedQuantity", "[hangars][reservations]")
{
    // Arrange
    const auto [amount, expected] = GENERATE(table<int, int>({{0, 2}, {1, 1}, {2, 0}}));
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("HANGAR", "crafts: 2"));
    auto &candidate = fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]");
    Craft pendingCraft(&candidate, &fixture.getBase(), 10);
    const std::vector<TransferRow> pending{createPendingRow(TRANSFER_CRAFT, &pendingCraft, amount)};

    // Act
    const int free = HangarCompatibility::getFreeHangarsCount(fixture.getBase(), &candidate, &pending, true);

    // Assert
    REQUIRE(free == expected);
}

TEST_CASE("HangarCompatibility_PendingNonCraftRows_DoesNotReserveHangar", "[hangars][reservations]")
{
    // Arrange
    const auto type = GENERATE(TRANSFER_ITEM, TRANSFER_SOLDIER, TRANSFER_SCIENTIST, TRANSFER_ENGINEER);
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("HANGAR", "crafts: 1\nhangarType: A"));
    auto &candidate = fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]");
    const std::vector<TransferRow> pending{createPendingRow(type, nullptr, 10)};

    // Act
    const int free = HangarCompatibility::getFreeHangarsCount(fixture.getBase(), &candidate, &pending);

    // Assert
    REQUIRE(free == 1);
}

TEST_CASE("HangarCompatibility_IncompatiblePendingCraft_ReturnsNoFreeCapacity", "[hangars][reservations]")
{
    // Arrange
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("HANGAR", "crafts: 1\nhangarType: A"));
    auto &candidate = fixture.rules.createCraft("CANDIDATE", "allowedHangarTypes: [A]");
    auto &incoming = fixture.rules.createCraft("INCOMING", "allowedHangarTypes: [B]");
    const std::vector<TransferRow> pending{createPendingRow(TRANSFER_CRAFT, &incoming, 1)};

    // Act
    const int free = HangarCompatibility::getFreeHangarsCount(fixture.getBase(), &candidate, &pending);

    // Assert
    REQUIRE(free == 0);
}

TEST_CASE("HangarCompatibility_CraftProduction_ReservesOnlyRemainingQuantityWithinCapacityBound",
          "[hangars][reservations]")
{
    // Arrange
    const auto [total, produced, expected] = GENERATE(table<int, int, int>({
        {6, 0, 0},
        {6, 5, 1},
        {6, 6, 2},
        {1000000, 0, 0},
    }));
    CAPTURE(total, produced);
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("HANGAR", "crafts: 2"));
    auto &candidate = fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]");
    fixture.addProduction(candidate, total, produced);

    // Act
    const int free = HangarCompatibility::getFreeHangarsCount(fixture.getBase(), &candidate);

    // Assert
    REQUIRE(free == expected);
}

TEST_CASE("HangarCompatibility_FullOrIncompatibleBase_RejectsCandidateAddition", "[hangars][matching]")
{
    // Arrange
    const auto existingYaml = GENERATE("allowedHangarTypes: [A]", "allowedHangarTypes: [B]");
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("HANGAR", "crafts: 1\nhangarType: A"));
    fixture.addCraft(fixture.rules.createCraft("EXISTING", existingYaml));
    auto &candidate = fixture.rules.createCraft("CANDIDATE", "allowedHangarTypes: [A]");

    // Act
    const bool lacks = HangarCompatibility::lacksCompatibleSlot(fixture.getBase(), &candidate);

    // Assert
    REQUIRE(lacks);
    REQUIRE(HangarCompatibility::getFreeHangarsCount(fixture.getBase(), &candidate) == 0);
}

TEST_CASE("HangarCompatibility_AssignmentNeedsReallocation_FitsEveryCraft", "[hangars][matching]")
{
    // Arrange
    BaseFixture fixture;
    MatchingScenario scenario{{"A", "B", "C"}, {{"A", "B"}, {"B", "C"}}, {"A", "B"}};
    auto &candidate = addMatchingScenario(fixture, scenario);

    // Act
    const int free = HangarCompatibility::getFreeHangarsCount(fixture.getBase(), &candidate);

    // Assert
    REQUIRE(free == 1);
    REQUIRE_FALSE(HangarCompatibility::lacksCompatibleSlot(fixture.getBase(), &candidate));
}

TEST_CASE("HangarCompatibility_DisplayAssignment_PrefersAlphabeticalTypedSlotsBeforeUntyped", "[hangars][display]")
{
    // Arrange
    BaseFixture fixture;
    auto &untyped = fixture.addFacility(fixture.rules.createFacility("UNTYPED", "crafts: 1"));
    auto &second = fixture.addFacility(fixture.rules.createFacility("B", "crafts: 1\nhangarType: B"));
    auto &first = fixture.addFacility(fixture.rules.createFacility("A", "crafts: 1\nhangarType: A"));
    auto &craft = fixture.addCraft(fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A, B]"));

    // Act
    const auto assignment = HangarCompatibility::calculateDisplayAssignment(fixture.getBase());

    // Assert
    REQUIRE(assignment.hangarCompatibilityEnabled);
    REQUIRE(assignment.getCraftForFacility(&first) == &craft);
    REQUIRE(assignment.getCraftForFacility(&second) == nullptr);
    REQUIRE(assignment.getCraftForFacility(&untyped) == nullptr);
}

TEST_CASE("HangarCompatibility_InvalidDisplayAssignment_UsesVisualFallbackWithoutClaimingValidity",
          "[hangars][display]")
{
    // Arrange
    BaseFixture fixture;
    auto &facility = fixture.addFacility(fixture.rules.createFacility("A", "crafts: 1\nhangarType: A"));
    auto &craft = fixture.addCraft(fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [B]"));

    // Act
    const auto assignment = HangarCompatibility::calculateDisplayAssignment(fixture.getBase());

    // Assert
    REQUIRE(assignment.getCraftForFacility(&facility) == &craft);
    REQUIRE(HangarCompatibility::hasInvalidAssignment(fixture.getBase()));
}

TEST_CASE("HangarCompatibility_DisplayAssignmentWithTransfer_MapsOnlyActualResidentCraft", "[hangars][display]")
{
    // Arrange
    BaseFixture fixture;
    auto &first = fixture.addFacility(fixture.rules.createFacility("A", "crafts: 1\nhangarType: A"));
    auto &second = fixture.addFacility(fixture.rules.createFacility("B", "crafts: 1\nhangarType: B"));
    auto &craft = fixture.addCraft(fixture.rules.createCraft("RESIDENT", "allowedHangarTypes: [B]"));
    fixture.addTransfer(fixture.rules.createCraft("INCOMING", "allowedHangarTypes: [A]"));

    // Act
    const auto assignment = HangarCompatibility::calculateDisplayAssignment(fixture.getBase());

    // Assert
    REQUIRE(assignment.craftByFacility.size() == 1);
    REQUIRE(assignment.getCraftForFacility(&second) == &craft);
    REQUIRE(assignment.getCraftForFacility(&first) == nullptr);
}

TEST_CASE("HangarCompatibility_RemoveRequiredTypedSlot_RejectsDespiteRemainingTotalCapacity", "[hangars][lifecycle]")
{
    // Arrange
    BaseFixture fixture;
    auto &removed = fixture.addFacility(fixture.rules.createFacility("A", "crafts: 1\nhangarType: A"));
    fixture.addFacility(fixture.rules.createFacility("B", "crafts: 1\nhangarType: B"));
    fixture.addCraft(fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]"));

    // Act
    const bool fits = HangarCompatibility::fitsAfterChange(fixture.getBase(), removed.getPlacement(), nullptr);

    // Assert
    REQUIRE_FALSE(fits);
}

TEST_CASE("HangarCompatibility_ReplaceTypedSlot_AcceptsOnlyCompatibleReplacement", "[hangars][lifecycle]")
{
    // Arrange
    const auto [replacementYaml, expected] = GENERATE(table<const char *, bool>({
        {"crafts: 1\nhangarType: A", true},
        {"crafts: 1\nhangarType: B", false},
        {"crafts: 1", true},
    }));
    BaseFixture fixture;
    auto &removed = fixture.addFacility(fixture.rules.createFacility("ORIGINAL", "crafts: 1\nhangarType: A"));
    fixture.addCraft(fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]"));
    auto &replacement = fixture.rules.createFacility("REPLACEMENT", replacementYaml);

    // Act
    const bool fits = HangarCompatibility::fitsAfterChange(fixture.getBase(), removed.getPlacement(), &replacement);

    // Assert
    REQUIRE(fits == expected);
}

TEST_CASE("HangarCompatibility_UpgradingExistingHangar_CountsPreviousFacilityCapacity", "[hangars][lifecycle]")
{
    // Arrange
    BaseFixture fixture;
    auto &upgrading = fixture.addFacility(fixture.rules.createFacility("HANGAR", "crafts: 1\nhangarType: A"), 10);
    upgrading.setIfHadPreviousFacility(true);
    fixture.addCraft(fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]"));

    // Act
    const bool fits = HangarCompatibility::fitsAfterChange(fixture.getBase(), BaseAreaSubset{}, nullptr);

    // Assert
    REQUIRE(fits);
}

TEST_CASE("Base_RemoveRequiredTypedHangar_ReturnsHangarsInUseError", "[hangars][integration]")
{
    // Arrange
    BaseFixture fixture;
    auto &removed = fixture.addFacility(fixture.rules.createFacility("A", "crafts: 1\nhangarType: A"));
    fixture.addFacility(fixture.rules.createFacility("B", "crafts: 1\nhangarType: B"));
    fixture.addCraft(fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]"));

    // Act
    const auto error = fixture.getBase().isAreaInUse(removed.getPlacement());

    // Assert
    REQUIRE(error == BPE_Used_Hangars);
}

TEST_CASE("HangarCompatibility_InvalidAssignmentRepeatedWarning_SuppressesSecondWarning", "[hangars][warnings]")
{
    // Arrange
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("A", "crafts: 1\nhangarType: A"));
    fixture.addCraft(fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [B]"));
    const bool firstWarning = HangarCompatibility::shouldWarn(fixture.getBase());

    // Act
    const bool secondWarning = HangarCompatibility::shouldWarn(fixture.getBase());

    // Assert
    REQUIRE(firstWarning);
    REQUIRE_FALSE(secondWarning);
}

TEST_CASE("HangarCompatibility_CorrectedAssignment_WarnsAgainWhenAssignmentBecomesInvalid", "[hangars][warnings]")
{
    // Arrange
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("A", "crafts: 1\nhangarType: A"));
    auto &craft = fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [B]");
    fixture.addCraft(craft);
    HangarCompatibility::shouldWarn(fixture.getBase());

    // Act
    fixture.addFacility(fixture.rules.createFacility("B", "crafts: 1\nhangarType: B"));
    const bool correctedWarning = HangarCompatibility::shouldWarn(fixture.getBase());
    fixture.addCraft(craft);
    const bool renewedWarning = HangarCompatibility::shouldWarn(fixture.getBase());

    // Assert
    REQUIRE_FALSE(correctedWarning);
    REQUIRE(renewedWarning);
}

TEST_CASE("HangarCompatibility_PrepareDamage_TracksResidentCraftAndExcludesAirborneCraft", "[hangars][lifecycle]")
{
    // Arrange
    const auto [status, expectedAttached] = GENERATE(table<const char *, bool>({
        {"STR_READY", true},
        {"STR_OUT", false},
    }));
    BaseFixture fixture;
    auto &facility = fixture.addFacility(fixture.rules.createFacility("A", "crafts: 1\nhangarType: A"));
    auto &craft = fixture.addCraft(fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]"), status);

    // Act
    HangarCompatibility::prepareDamage(fixture.getBase(), facility);

    // Assert
    REQUIRE((facility.getCraftForDrawing() == &craft) == expectedAttached);
}

TEST_CASE("HangarCompatibility_PrepareDestruction_TracksResidentCraftAndExcludesAirborneCraft", "[hangars][lifecycle]")
{
    // Arrange
    const auto [status, expectedAttached] = GENERATE(table<const char *, bool>({
        {"STR_READY", true},
        {"STR_OUT", false},
    }));
    BaseFixture fixture;
    auto &facility = fixture.addFacility(fixture.rules.createFacility("A", "crafts: 1\nhangarType: A"));
    auto &craft = fixture.addCraft(fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]"), status);

    // Act
    HangarCompatibility::prepareDestruction(fixture.getBase(), facility);

    // Assert
    REQUIRE((facility.getCraftForDrawing() == &craft) == expectedAttached);
}

TEST_CASE("HangarCompatibility_DamagedHangarReplacement_KeepsOnlyCompatibleCraft", "[hangars][lifecycle]")
{
    // Arrange
    const auto [replacementYaml, expected] = GENERATE(table<const char *, bool>({
        {"crafts: 1\nhangarType: A", true},
        {"crafts: 1\nhangarType: B", false},
        {"crafts: 1", true},
    }));
    BaseFixture fixture;
    auto &damaged = fixture.addFacility(fixture.rules.createFacility("A", "crafts: 1\nhangarType: A"));
    auto &craft = fixture.addCraft(fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]"));
    damaged.setCraftForDrawing(&craft);
    auto &replacementRule = fixture.rules.createFacility("REPLACEMENT", replacementYaml);
    BaseFacility replacement(&replacementRule, &fixture.getBase());

    // Act
    const bool keeps = HangarCompatibility::canKeepAfterDamage(fixture.getBase(), damaged, replacement);

    // Assert
    REQUIRE(keeps == expected);
}

TEST_CASE("Base_DestroyTypedHangar_RemovesItsAssignedResidentCraft", "[hangars][integration]")
{
    // Arrange
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("A", "crafts: 1\nhangarType: A"));
    fixture.addFacility(fixture.rules.createFacility("B", "crafts: 1\nhangarType: B"));
    fixture.addCraft(fixture.rules.createCraft("CRAFT_A", "allowedHangarTypes: [A]"));
    auto &survivor = fixture.addCraft(fixture.rules.createCraft("CRAFT_B", "allowedHangarTypes: [B]"));

    // Act
    fixture.getBase().destroyFacility(fixture.getBase().getFacilities()->begin());

    // Assert
    REQUIRE(fixture.getBase().getFacilities()->size() == 1);
    REQUIRE(fixture.getBase().getCrafts()->size() == 1);
    REQUIRE(fixture.getBase().getCrafts()->front() == &survivor);
}

TEST_CASE("Base_DestroyTypedHangar_PreservesAirborneCraft", "[hangars][integration]")
{
    // Arrange
    BaseFixture fixture;
    fixture.addFacility(fixture.rules.createFacility("A", "crafts: 1\nhangarType: A"));
    auto &craft = fixture.addCraft(fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]"), "STR_OUT");

    // Act
    fixture.getBase().destroyFacility(fixture.getBase().getFacilities()->begin());

    // Assert
    REQUIRE(fixture.getBase().getFacilities()->empty());
    REQUIRE(fixture.getBase().getCrafts()->size() == 1);
    REQUIRE(fixture.getBase().getCrafts()->front() == &craft);
}

TEST_CASE("Base_DamageTypedHangar_KeepsCraftOnlyInCompatibleReplacement", "[hangars][integration]")
{
    // Arrange
    const auto [replacementYaml, expectedCrafts] = GENERATE(table<const char *, size_t>({
        {"crafts: 1\nhangarType: A", 1},
        {"crafts: 1\nhangarType: B", 0},
        {"crafts: 1", 1},
    }));
    BaseFixture fixture;
    auto &originalRule = fixture.rules.createFacility("ORIGINAL", "crafts: 1\nhangarType: A");
    auto &replacementRule = fixture.rules.createFacility("REPLACEMENT", replacementYaml);
    fixture.rules.setDestroyedFacility(originalRule, replacementRule);
    auto &original = fixture.addFacility(originalRule);
    fixture.addCraft(fixture.rules.createCraft("CRAFT", "allowedHangarTypes: [A]"));

    // Act
    fixture.getBase().damageFacility(&original);

    // Assert
    REQUIRE(fixture.getBase().getFacilities()->size() == 1);
    REQUIRE(fixture.getBase().getFacilities()->front()->getRules() == &replacementRule);
    REQUIRE(fixture.getBase().getCrafts()->size() == expectedCrafts);
}
