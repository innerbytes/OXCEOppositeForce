#include "Fixtures.h"
#include "Savegame/MissionSite.h"
#include "Savegame/Waypoint.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

TEST_CASE("Ufo_SetDetectedOnHiddenUfo_ClearsRadarFlagAndPreservesHyperwaveFlag", "[hidden-ufo][lifecycle]")
{
    // Arrange
    const bool detected = GENERATE(false, true);
    DetectionFixture fixture("isHidden: true");
    fixture.getUfo().ofRevealForDebug();

    // Act
    fixture.getUfo().setDetected(detected);

    // Assert
    REQUIRE_FALSE(fixture.getUfo().getDetected());
    REQUIRE(fixture.getUfo().getHyperDetected());
}

TEST_CASE("Ufo_SetHyperDetectedOnHiddenUfo_ClearsHyperwaveFlagAndPreservesRadarFlag", "[hidden-ufo][lifecycle]")
{
    // Arrange
    const bool detected = GENERATE(false, true);
    DetectionFixture fixture("isHidden: true");
    fixture.getUfo().ofRevealForDebug();

    // Act
    fixture.getUfo().setHyperDetected(detected);

    // Assert
    REQUIRE(fixture.getUfo().getDetected());
    REQUIRE_FALSE(fixture.getUfo().getHyperDetected());
}

TEST_CASE("Ufo_NormalSettersOnVisibleUfo_StoresDetectionFlags", "[hidden-ufo][lifecycle]")
{
    // Arrange
    const bool radar = GENERATE(false, true);
    const bool hyperwave = GENERATE(false, true);
    DetectionFixture fixture("{}");

    // Act
    fixture.getUfo().setDetected(radar);
    fixture.getUfo().setHyperDetected(hyperwave);

    // Assert
    REQUIRE(fixture.getUfo().getDetected() == radar);
    REQUIRE(fixture.getUfo().getHyperDetected() == hyperwave);
}

TEST_CASE("Ufo_HiddenUfoDuringNormalPlay_HasNoDetectionFlagsOrMarker", "[hidden-ufo][lifecycle]")
{
    // Arrange
    DetectionFixture fixture("isHidden: true");

    // Act
    fixture.getUfo().setDetected(true);
    fixture.getUfo().setHyperDetected(true);

    // Assert
    REQUIRE_FALSE(fixture.getUfo().getDetected());
    REQUIRE_FALSE(fixture.getUfo().getHyperDetected());
    REQUIRE(fixture.getUfo().getMarker() == -1);
}

TEST_CASE("Ufo_CrashedStatus_RevealsOnlyVisibleRules", "[hidden-ufo][lifecycle]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, bool>({
        {"isHidden: true", false},
        {"isHidden: false", true},
    }));
    DetectionFixture fixture(yaml);
    fixture.getUfo().setStatus(Ufo::CRASHED);

    // Act
    fixture.getUfo().think();

    // Assert
    REQUIRE(fixture.getUfo().getDetected() == expected);
}

TEST_CASE("Ufo_DebugRevealOnHiddenUfo_SetsBothFlagsAndShowsMarker", "[hidden-ufo][debug]")
{
    // Arrange
    DetectionFixture fixture("isHidden: true");

    // Act
    fixture.getUfo().ofRevealForDebug();

    // Assert
    REQUIRE(fixture.getUfo().getDetected());
    REQUIRE(fixture.getUfo().getHyperDetected());
    REQUIRE(fixture.getUfo().getMarker() != -1);
}

TEST_CASE("Ufo_CrashedAfterDebugReveal_PreservesBothDetectionFlags", "[hidden-ufo][debug]")
{
    // Arrange
    DetectionFixture fixture("isHidden: true");
    fixture.getUfo().setStatus(Ufo::CRASHED);
    fixture.getUfo().ofRevealForDebug();

    // Act
    fixture.getUfo().think();

    // Assert
    REQUIRE(fixture.getUfo().getDetected());
    REQUIRE(fixture.getUfo().getHyperDetected());
}

TEST_CASE("Ufo_ReloadAfterDebugReveal_ClearsFlagsOnlyForHiddenRules", "[hidden-ufo][save]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, bool>({
        {"isHidden: true", false},
        {"isHidden: false", true},
    }));
    DetectionFixture fixture(yaml);
    fixture.getUfo().setStatus(Ufo::CRASHED);
    fixture.getUfo().ofRevealForDebug();
    YAML::YamlRootNodeWriter snapshot;
    fixture.getUfo().save(snapshot, fixture.base.rules.getShared(), true);
    Ufo reloaded(fixture.getUfo().getRules(), 2);

    // Act
    reloaded.load(snapshot.toReader(), fixture.base.rules.getShared(), fixture.base.rules.getMod(), fixture.save);

    // Assert
    REQUIRE(reloaded.getDetected() == expected);
    REQUIRE(reloaded.getHyperDetected() == expected);
    REQUIRE((reloaded.getMarker() != -1) == expected);
}

TEST_CASE("Ufo_LoadWithLegacySavedDetectionFlags_ClearsFlagsForHiddenRule", "[hidden-ufo][save]")
{
    // Arrange
    DetectionFixture fixture("isHidden: true");
    auto &visibleRule = fixture.base.rules.createUfo("VISIBLE");
    Ufo visible(&visibleRule, 2);
    visible.setDetected(true);
    visible.setHyperDetected(true);
    YAML::YamlRootNodeWriter snapshot;
    visible.save(snapshot, fixture.base.rules.getShared(), true);

    // Act
    fixture.getUfo().load(snapshot.toReader(), fixture.base.rules.getShared(), fixture.base.rules.getMod(),
                          fixture.save);

    // Assert
    REQUIRE_FALSE(fixture.getUfo().getDetected());
    REQUIRE_FALSE(fixture.getUfo().getHyperDetected());
    REQUIRE(fixture.getUfo().getMarker() == -1);
}

TEST_CASE("Ufo_CleanupAfterDebugReveal_ClearsBothFlagsAndMarker", "[hidden-ufo][debug]")
{
    // Arrange
    DetectionFixture fixture("isHidden: true");
    fixture.getUfo().ofRevealForDebug();

    // Act
    fixture.getUfo().setDetected(false);
    fixture.getUfo().setHyperDetected(false);

    // Assert
    REQUIRE_FALSE(fixture.getUfo().getDetected());
    REQUIRE_FALSE(fixture.getUfo().getHyperDetected());
    REQUIRE(fixture.getUfo().getMarker() == -1);
}

TEST_CASE("Ufo_LandedHiddenUfo_AdvancesLandingCountdown", "[hidden-ufo][mission]")
{
    // Arrange
    DetectionFixture fixture("isHidden: true");
    fixture.getUfo().setStatus(Ufo::LANDED);
    fixture.getUfo().setSecondsRemaining(10);

    // Act
    fixture.getUfo().think();

    // Assert
    REQUIRE(fixture.getUfo().getSecondsRemaining() == 5);
    REQUIRE_FALSE(fixture.getUfo().getDetected());
}

TEST_CASE("Ufo_FlyingHiddenUfo_MovesTowardDestination", "[hidden-ufo][mission]")
{
    // Arrange
    DetectionFixture fixture("isHidden: true\nspeedMax: 60000");
    auto *destination = new Waypoint();
    destination->setLongitude(0.001);
    fixture.getUfo().setDestination(destination);
    fixture.getUfo().setSpeed(60000);

    // Act
    fixture.getUfo().think();

    // Assert
    REQUIRE(fixture.getUfo().getLongitude() > 0);
    REQUIRE_FALSE(fixture.getUfo().getDetected());
}

TEST_CASE("Ufo_HiddenUfoReceivesHuntingTarget_KeepsTargetWithoutRevealing", "[hidden-ufo][mission]")
{
    // Arrange
    DetectionFixture fixture("isHidden: true");
    fixture.getUfo().setDestination(new Waypoint());

    // Act
    fixture.getUfo().setTargetedXcomCraft(&fixture.getCraft());
    fixture.getUfo().setDetected(true);

    // Assert
    REQUIRE(fixture.getUfo().isHunting());
    REQUIRE(fixture.getUfo().getTargetedXcomCraft() == &fixture.getCraft());
    REQUIRE_FALSE(fixture.getUfo().getDetected());
}
