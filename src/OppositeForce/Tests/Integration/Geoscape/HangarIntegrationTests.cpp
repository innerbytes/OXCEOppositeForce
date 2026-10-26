#include "Fixtures.h"
#include "OppositeForce/Geoscape/Rules/HangarRules.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

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
