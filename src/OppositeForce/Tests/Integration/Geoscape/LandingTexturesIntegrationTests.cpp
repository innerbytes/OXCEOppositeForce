#include <filesystem>
#include <functional>
#include <memory>

#include "Engine/FileMap.h"
#include "Engine/Game.h"
#include "Engine/Options.h"
#include "Engine/SurfaceSet.h"
#include "Fixtures.h"
#include "Geoscape/Globe.h"
#include "LandingFixtures.h"
#include "LandingRuleFixtures.h"
#include "Mod/AlienRace.h"
#include "Mod/RuleGlobe.h"
#include "Mod/Texture.h"
#include "Mod/UfoTrajectory.h"
#include "OppositeForce/Battlescape/Game/GlobalTerrain.h"
#include "OppositeForce/Geoscape/Game/LandingTextures.h"
#include "Savegame/MissionSite.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

TEST_CASE("AlienMission_OrdinaryLanding_UsesTextureFilter", "[landing-textures][integration]")
{
    // Arrange
    LandingFixture fixture;
    Ufo ufo(&fixture.ufoRule, 1);

    // Act
    const auto position = fixture.mission.getLandPoint(*fixture.globe, fixture.region, 0, ufo);

    // Assert
    REQUIRE_FALSE(fixture.mission.ofLandingSelectionFailed);
    REQUIRE(fixture.getTexture(position) == 1);
}

TEST_CASE("AlienMission_EmptyExclusions_PreservesLegacyCoordinatesAndRandomness", "[landing-textures][integration]")
{
    // Arrange
    const bool forSite = GENERATE(false, true);
    LandingFixture fixture("excludedLandingTextures: []");
    Ufo ufo(&fixture.ufoRule, 1);
    const auto seed = RNG::getSeed();
    std::pair<double, double> expected;
    if (forSite)
    {
        RNG::percent(0);
        expected = fixture.region.getRandomPoint(0, 0);
    }
    else
    {
        RNG::percent(0);
        expected = fixture.region.getRandomPoint(0);
    }
    const auto expectedSeed = RNG::getSeed();
    RNG::setSeed(seed);

    // Act
    const auto position = forSite
                              ? fixture.mission.getLandPointForMissionSite(*fixture.globe, fixture.region, 0, 0, ufo)
                              : fixture.mission.getLandPoint(*fixture.globe, fixture.region, 0, ufo);

    // Assert
    REQUIRE(position == expected);
    REQUIRE(RNG::getSeed() == expectedSeed);
    REQUIRE_FALSE(fixture.mission.ofLandingSelectionFailed);
}

TEST_CASE("AlienMission_FailedSpawn_DoesNotCreateSiteAndAdvancesWaveCounters", "[landing-textures][integration]")
{
    // Arrange
    LandingFixture fixture("excludedLandingTextures: [1, 9, 12]");
    loadMissionRule(
        fixture.rule,
        "waves:\n"
        "  - {ufo: HIDDEN, count: 1, trajectory: LAND, timer: 30, objective: true, objectiveOnTheLandingSite: true}\n"
        "  - {ufo: HIDDEN, count: 1, trajectory: LAND, timer: 30, objective: true, objectiveOnTheLandingSite: true}");

    // Act
    fixture.mission.think(fixture.getGame(), *fixture.globe);

    // Assert
    REQUIRE(fixture.save.getUfos()->empty());
    REQUIRE(fixture.save.getMissionSites()->empty());
    REQUIRE(fixture.mission._liveUfos == 0);
    REQUIRE(fixture.mission._nextWave == 1);
    REQUIRE(fixture.mission._nextUfoCounter == 0);
    REQUIRE_FALSE(fixture.mission.isOver());
    REQUIRE_FALSE(fixture.mission._interrupted);

    // Arrange
    loadMissionRule(fixture.rule, "excludedLandingTextures: [9, 12]");
    fixture.mission.setWaveCountdown(0);

    // Act
    fixture.mission.think(fixture.getGame(), *fixture.globe);

    // Assert
    REQUIRE(fixture.save.getUfos()->size() == 1);
    REQUIRE(fixture.mission._liveUfos == 1);
    REQUIRE_FALSE(fixture.mission.ofLandingSelectionFailed);
}

TEST_CASE("AlienMission_FailedInFlightLanding_RemovesUfoWithoutCreatingSite", "[landing-textures][integration]")
{
    // Arrange
    LandingFixture fixture;
    std::unique_ptr<Ufo> ufo(fixture.spawnUfo());
    REQUIRE(ufo);
    loadMissionRule(fixture.rule, "excludedLandingTextures: [1, 9, 12]");

    // Act
    fixture.mission.ufoReachedWaypoint(*ufo, fixture.getGame(), *fixture.globe);

    // Assert
    REQUIRE(ufo->getStatus() == Ufo::DESTROYED);
    REQUIRE(fixture.save.getMissionSites()->empty());
    ufo.reset();
    REQUIRE(fixture.mission._liveUfos == 0);
}

TEST_CASE("AlienMission_FailedLandingAcrossSpawnBranches_ReleasesUfoAndPendingWaypoint",
          "[landing-textures][integration]")
{
    // Arrange
    const auto [objective, waypoints] = GENERATE(table<int, const char *>({
        {0, "[[0, 1, 100], [0, 0, 100]]"},
        {0, "[[1, 1, 100], [0, 1, 100], [0, 0, 100]]"},
        {5, "[[0, 0, 100], [0, 1, 100]]"},
        {5, "[[0, 1, 100], [0, 0, 100]]"},
        {4, "[[0, 0, 100], [0, 1, 100]]"},
    }));
    CAPTURE(objective, waypoints);
    LandingFixture fixture("excludedLandingTextures: [1, 9, 12]");
    loadMissionRule(fixture.rule, "objective: " + std::to_string(objective) +
                                      "\nskipScoutingPhase: true\nwaves:\n"
                                      "  - {ufo: HIDDEN, count: 1, trajectory: LAND, timer: 30, objective: false}");
    fixture.region._missionZones.push_back(fixture.region._missionZones.front());
    fixture.loadTrajectory(waypoints);
    if (objective == OBJECTIVE_RETALIATION)
    {
        auto *base = new Base(&fixture.rules.getMod());
        base->setLongitude(Deg2Rad(1.5));
        base->setLatitude(Deg2Rad(1.5));
        fixture.save.getBases()->push_back(base);
        fixture.rules.getMod()._ufoTrajectories[UfoTrajectory::RETALIATION_ASSAULT_RUN] =
            new UfoTrajectory(UfoTrajectory::RETALIATION_ASSAULT_RUN);
    }

    // Act
    std::unique_ptr<Ufo> ufo(fixture.spawnUfo());

    // Assert
    REQUIRE_FALSE(ufo);
    REQUIRE(fixture.mission.ofLandingSelectionFailed);
    REQUIRE(fixture.mission._liveUfos == 0);
    REQUIRE(fixture.save.getMissionSites()->empty());
}

TEST_CASE("AlienMission_DirectSiteSpawn_IgnoresLandingExclusionsAndResetsPreviousFailure",
          "[landing-textures][integration]")
{
    // Arrange
    LandingFixture fixture;
    fixture.loadRegion("[[1, 1, 1, 1]]");
    fixture.rules.createDeployment("DIRECT", "markerName: DIRECT\nuseGlobalTerrain: true");
    loadMissionRule(fixture.rule, "waves:\n  - {ufo: DIRECT, count: 1, trajectory: LAND, timer: 30, objective: true}");
    fixture.mission.ofLandingSelectionFailed = true;

    // Act
    fixture.mission.think(fixture.getGame(), *fixture.globe);

    // Assert
    REQUIRE_FALSE(fixture.mission.ofLandingSelectionFailed);
    REQUIRE(fixture.save.getUfos()->empty());
    REQUIRE(fixture.save.getMissionSites()->size() == 1);
    REQUIRE(fixture.save.getMissionSites()->front()->getLongitude() == Deg2Rad(1));
    REQUIRE(fixture.save.getMissionSites()->front()->getLatitude() == Deg2Rad(1));
}

TEST_CASE("AlienMission_FallbackArea_SurvivesSaveLoadAndCreatesGlobalTerrainSite", "[landing-textures][integration]")
{
    // Arrange
    LandingFixture fixture;
    std::unique_ptr<Ufo> ufo(fixture.spawnUfo());
    REQUIRE(ufo);
    const auto landing = std::make_pair(ufo->getDestination()->getLongitude(), ufo->getDestination()->getLatitude());
    YAML::YamlRootNodeWriter snapshot;
    fixture.mission.save(snapshot);
    AlienMission reloaded(fixture.rule);

    // Act
    reloaded.load(snapshot.toReader(), fixture.save, &fixture.rules.getMod());
    ufo->setLongitude(landing.first);
    ufo->setLatitude(landing.second);
    fixture.mission.ufoReachedWaypoint(*ufo, fixture.getGame(), *fixture.globe);

    // Assert
    REQUIRE(reloaded._missionSiteZoneArea == 1);
    REQUIRE(fixture.save.getMissionSites()->size() == 1);
    auto *site = fixture.save.getMissionSites()->front();
    REQUIRE(site->getLongitude() == landing.first);
    REQUIRE(site->getLatitude() == landing.second);
    REQUIRE(site->getCity() == "FIELD");
    REQUIRE(fixture.getTexture(landing) == 1);
    auto &terrain = fixture.rules.createTerrain("FIELD", "{}");
    fixture.loadGlobe("textures: [{id: 1, terrain: [{name: FIELD}]}]");
    REQUIRE(GlobalTerrain::selectTerrain(fixture.rules.getMod(), *site->getDeployment(), nullptr,
                                         fixture.rules.getMod().getGlobe()->getTexture(1), site) == &terrain);
}

TEST_CASE("AlienMission_NextLandingChangesArea_PreservesArrivingSiteMetadata", "[landing-textures][integration]")
{
    // Arrange
    LandingFixture fixture("excludedLandingTextures: [12]");
    fixture.mission.setMissionSiteZoneArea(1);
    std::unique_ptr<Ufo> ufo(fixture.spawnUfo());
    REQUIRE(ufo);
    ufo->setLongitude(ufo->getDestination()->getLongitude());
    ufo->setLatitude(ufo->getDestination()->getLatitude());
    fixture.loadGlobe("textures: [{id: 9, fakeUnderwater: true}]");
    fixture.rules.loadRule(fixture.ufoRule, "fakeWaterLandingChance: 100");

    // Act
    fixture.mission.ufoReachedWaypoint(*ufo, fixture.getGame(), *fixture.globe);

    // Assert
    REQUIRE(fixture.mission._missionSiteZoneArea == 0);
    REQUIRE(fixture.save.getMissionSites()->size() == 1);
    REQUIRE(fixture.save.getMissionSites()->front()->getCity() == "FIELD");
}

TEST_CASE("AlienMission_ConcurrentUfosSelectDifferentAreas_PreservesEachLandingMetadata",
          "[landing-textures][integration]")
{
    // Arrange
    LandingFixture fixture("excludedLandingTextures: [12]");
    std::unique_ptr<Ufo> first(fixture.spawnUfo());
    fixture.mission.setMissionSiteZoneArea(1);
    std::unique_ptr<Ufo> second(fixture.spawnUfo());
    REQUIRE(first);
    REQUIRE(second);
    first->setLongitude(first->getDestination()->getLongitude());
    first->setLatitude(first->getDestination()->getLatitude());

    // Act
    fixture.mission.ufoReachedWaypoint(*first, fixture.getGame(), *fixture.globe);

    // Assert
    REQUIRE(fixture.save.getMissionSites()->size() == 1);
    REQUIRE(fixture.save.getMissionSites()->front()->getCity() == "ICE");
}
