#include <filesystem>
#include <functional>
#include <memory>

#include "Engine/FileMap.h"
#include "Engine/Game.h"
#include "Engine/Options.h"
#include "Engine/SurfaceSet.h"
#include "Fixtures.h"
#include "Geoscape/Globe.h"
#include "Mod/AlienRace.h"
#include "Mod/RuleGlobe.h"
#include "Mod/Texture.h"
#include "Mod/UfoTrajectory.h"
#include "OppositeForce/Battlescape/GlobalTerrain.h"
#include "OppositeForce/Geoscape/LandingTextures.h"
#include "Savegame/MissionSite.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

namespace
{
    void loadMissionRule(RuleAlienMission &rule, const std::string &yaml)
    {
        YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "landing mission rule");
        rule.load(reader);
    }

    class HeadlessSettings
    {
        std::vector<std::function<void()>> _restore;

        template <typename T>
        void setOption(T &option, T value)
        {
            _restore.push_back([&option, previous = option]() { option = previous; });
            option = value;
        }

    public:
        HeadlessSettings()
        {
            // The test process never creates a native window or plays audio.
            static char video[] = "SDL_VIDEODRIVER=dummy";
            static char audio[] = "SDL_AUDIODRIVER=dummy";
            SDL_putenv(video);
            SDL_putenv(audio);
            setOption(Options::displayWidth, 320);
            setOption(Options::displayHeight, 200);
            setOption(Options::baseXResolution, 320);
            setOption(Options::baseYResolution, 200);
            setOption(Options::baseXGeoscape, 320);
            setOption(Options::baseYGeoscape, 200);
            setOption(Options::useOpenGL, false);
            setOption(Options::audioSampleRate, 11025);
            setOption(Options::audioChunkSize, 512);
            setOption(Options::mute, true);
            setOption(Options::reload, false);
            const auto previousDataFolder = Options::getDataFolder();
            _restore.push_back([previousDataFolder]() { Options::setDataFolder(previousDataFolder); });
            const auto repository =
                std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path();
            Options::setDataFolder((repository / "bin").string() + "/");
            FileMap::clear(false, false);
        }

        ~HeadlessSettings()
        {
            FileMap::clear(true, false);
            for (auto &restore : _restore) restore();
        }
    };

    class LandingFixture
    {
        HeadlessSettings _settings;
        std::unique_ptr<Game> _game;

    public:
        RulesFixture rules;
        SavedGame save;
        RuleAlienMission &rule;
        RuleRegion &region;
        RuleUfo &ufoRule;
        UfoTrajectory &trajectory;
        AlienMission mission;
        std::unique_ptr<Globe> globe;

        explicit LandingFixture(const std::string &exclusions = "excludedLandingTextures: [9, 12]")
            : rule(rules.createAlienMission("RESEARCH",
                                            "objective: 3\nspawnZone: 0\nsiteType: SITE\n" + exclusions +
                                                "\nwaves:\n  - {ufo: HIDDEN, count: 1, trajectory: LAND, timer: 30, "
                                                "objective: true, objectiveOnTheLandingSite: true}\n")),
              region(*new RuleRegion("REGION")),
              ufoRule(rules.createUfo("HIDDEN", "isHidden: true\nspeedMax: 100")),
              trajectory(*new UfoTrajectory("LAND")),
              mission(rule)
        {
            auto &mod = rules.getMod();
            mod._regions["REGION"] = &region;
            mod._alienRaces["RACE"] = new AlienRace("RACE", 0);
            mod._ufoTrajectories["LAND"] = &trajectory;
            mod._sets["TEXTURE.DAT"] = new SurfaceSet(1, 1);
            mod._sets["GlobeMarkers"] = new SurfaceSet(1, 1);
            rules.createDeployment("SITE", "markerName: SITE\nuseGlobalTerrain: true");
            loadRegion("[[1, 2, 1, 2, 0, ICE], [11, 12, 1, 2, 0, FIELD]]");
            loadGlobe("polygons:\n  - [9, 0, 0, 5, 0, 5, 5, 0, 5]\n  - [1, 10, 0, 15, 0, 15, 5, 10, 5]");
            loadTrajectory("[[0, 1, 100], [0, 0, 100]]");
            mission.setRegion("REGION", mod);
            mission.setRace("RACE");
            mission.setMissionSiteZoneArea(0);
            _game = std::make_unique<Game>("Headless landing tests");
            Options::mute = true;
            _game->_mod = &mod;
            _game->_save = &save;
            globe = std::make_unique<Globe>(_game.get(), 32, 32, 64, 64);
        }

        ~LandingFixture()
        {
            globe.reset();
            // Game normally owns these; this fixture owns them separately.
            _game->_mod = nullptr;
            _game->_save = nullptr;
            _game.reset();
            for (auto *ufo : *save.getUfos()) delete ufo;
            save.getUfos()->clear();
        }

        Game &getGame()
        {
            return *_game;
        }

        void loadRegion(const std::string &areas, const std::string &bounds = "[[0, 20, 0, 10]]")
        {
            YAML::YamlRootNodeReader reader(
                YAML::YamlString("deleteOldAreas: true\nareas: " + bounds + "\nmissionZones:\n  - " + areas),
                "landing region");
            region.load(reader, &rules.getMod());
        }

        void loadGlobe(const std::string &yaml)
        {
            YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "landing globe");
            rules.getMod().getGlobe()->load(reader, &rules.getMod());
        }

        void loadTrajectory(const std::string &waypoints)
        {
            YAML::YamlRootNodeReader reader(YAML::YamlString("waypoints: " + waypoints), "landing trajectory");
            trajectory.load(reader);
        }

        int getTexture(const std::pair<double, double> &position)
        {
            int texture, shade;
            globe->getPolygonTextureAndShade(position.first, position.second, &texture, &shade);
            return texture;
        }

        Ufo *spawnUfo()
        {
            mission.ofLandingSelectionFailed = false;
            return mission.spawnUfo(save, rules.getMod(), *globe, rule.getWave(0), trajectory);
        }
    };
}  // namespace

TEST_CASE("RuleAlienMission_ExcludedLandingTextures_LoadsDefaultsAndInheritance", "[landing-textures][rules]")
{
    // Arrange
    const auto [yaml, expected] = GENERATE(table<const char *, std::vector<int>>({
        {"{}", {}},
        {"excludedLandingTextures: [9, 12]", {9, 12}},
        {"refNode: {excludedLandingTextures: [9, 12]}", {9, 12}},
        {"refNode: {excludedLandingTextures: [9, 12]}\nexcludedLandingTextures: [3]", {3}},
        {"refNode: {excludedLandingTextures: [9, 12]}\nexcludedLandingTextures: []", {}},
    }));
    RuleAlienMission rule("RULE");

    // Act
    loadMissionRule(rule, yaml);

    // Assert
    REQUIRE(rule.ofGetExcludedLandingTextures() == expected);
}

TEST_CASE("RuleAlienMission_ExcludedLandingTextureLayers_PreservesReplacesOrClears", "[landing-textures][rules]")
{
    // Arrange
    const auto [layer, expected] = GENERATE(table<const char *, std::vector<int>>({
        {"points: 1", {9, 12}},
        {"excludedLandingTextures: [2]", {2}},
        {"excludedLandingTextures: []", {}},
    }));
    RuleAlienMission rule("RULE");
    loadMissionRule(rule, "excludedLandingTextures: [9, 12]");

    // Act
    loadMissionRule(rule, layer);

    // Assert
    REQUIRE(rule.ofGetExcludedLandingTextures() == expected);
}

TEST_CASE("LandingTextures_ExcludedInitialArea_SelectsAllowedAreaUsingActualTexture", "[landing-textures]")
{
    // Arrange
    LandingFixture fixture;
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = true;
    int selectedArea = 0;

    // Act
    const auto position = LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, 0, ufo, fixture.rule,
                                                              failed, selectedArea);

    // Assert
    REQUIRE_FALSE(failed);
    REQUIRE(selectedArea == 1);
    REQUIRE(fixture.getTexture(position) == 1);
    REQUIRE(fixture.region.getMissionZones()[0].areas[0].texture == 0);
}

TEST_CASE("LandingTextures_MixedTextureArea_RejectsIceCoordinates", "[landing-textures]")
{
    // Arrange
    LandingFixture fixture;
    fixture.loadRegion("[[1, 12, 1, 2]]");
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = true;
    int selectedArea = 0;

    // Act
    const auto position = LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, 0, ufo, fixture.rule,
                                                              failed, selectedArea);

    // Assert
    REQUIRE_FALSE(failed);
    REQUIRE(selectedArea == 0);
    REQUIRE(fixture.getTexture(position) == 1);
}

TEST_CASE("LandingTextures_AllAreasExcluded_ReturnsFailureWithoutForcedLanding", "[landing-textures]")
{
    // Arrange
    LandingFixture fixture("excludedLandingTextures: [1, 9, 12]");
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = false;
    int selectedArea = 0;

    // Act
    LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, 0, ufo, fixture.rule, failed, selectedArea);

    // Assert
    REQUIRE(failed);
    REQUIRE(selectedArea == 0);
}

TEST_CASE("LandingTextures_PointAreas_RejectsExcludedPointAndPreservesMissionSiteArea", "[landing-textures]")
{
    // Arrange
    LandingFixture fixture;
    fixture.loadRegion("[[1, 1, 1, 1], [11, 11, 1, 1]]");
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = true;
    int selectedArea = 37;

    // Act
    const auto position = LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, -1, ufo, fixture.rule,
                                                              failed, selectedArea);

    // Assert
    REQUIRE_FALSE(failed);
    REQUIRE(selectedArea == 37);
    REQUIRE(fixture.getTexture(position) == 1);
}

TEST_CASE("LandingTextures_FakeWaterPreference_PreservesChosenSurfaceAcrossAreas", "[landing-textures]")
{
    // Arrange
    const bool wantsFakeWater = GENERATE(false, true);
    LandingFixture fixture("excludedLandingTextures: [12]");
    fixture.loadGlobe("textures: [{id: 1, fakeUnderwater: true}]");
    fixture.rules.loadRule(fixture.ufoRule, "fakeWaterLandingChance: " + std::to_string(wantsFakeWater ? 100 : 0));
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = true;
    int selectedArea = 0;

    // Act
    const auto position = LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, 0, ufo, fixture.rule,
                                                              failed, selectedArea);

    // Assert
    REQUIRE_FALSE(failed);
    REQUIRE(fixture.globe->insideFakeUnderwaterTexture(position.first, position.second) == wantsFakeWater);
    REQUIRE(selectedArea == (wantsFakeWater ? 1 : 0));
}

TEST_CASE("LandingTextures_RegionBoundary_OnlyMissionSiteLandingCanIgnoreIt", "[landing-textures]")
{
    // Arrange
    const bool forSite = GENERATE(false, true);
    LandingFixture fixture;
    fixture.loadRegion("[[11, 12, 1, 2]]", "[[0, 5, 0, 5]]");
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = false;
    int selectedArea = 0;

    // Act
    LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, forSite ? 0 : -1, ufo, fixture.rule, failed,
                                        selectedArea);

    // Assert
    REQUIRE(failed == !forSite);
}

TEST_CASE("LandingTextures_AreaFallback_RollsFakeWaterPreferenceOnlyOnce", "[landing-textures]")
{
    // Arrange
    const uint64_t seed = GENERATE(1, 2, 3, 4);
    LandingFixture fixture;
    fixture.loadRegion("[[1, 2, 1, 2], [11, 12, 1, 2], [21, 22, 1, 2]]", "[[0, 30, 0, 10]]");
    fixture.loadGlobe(
        "polygons:\n"
        "  - [9, 0, 0, 5, 0, 5, 5, 0, 5]\n"
        "  - [1, 10, 0, 15, 0, 15, 5, 10, 5]\n"
        "  - [2, 20, 0, 25, 0, 25, 5, 20, 5]\n"
        "textures: [{id: 1, fakeUnderwater: true}]");
    fixture.rules.loadRule(fixture.ufoRule, "fakeWaterLandingChance: 50");
    Ufo ufo(&fixture.ufoRule, 1);
    bool failed = true;
    int selectedArea = 0;
    RNG::setSeed(seed);
    const bool wantsFakeWater = RNG::percent(50);
    RNG::setSeed(seed);

    // Act
    LandingTextures::selectLandingPoint(*fixture.globe, fixture.region, 0, 0, ufo, fixture.rule, failed, selectedArea);

    // Assert
    REQUIRE_FALSE(failed);
    REQUIRE(selectedArea == (wantsFakeWater ? 1 : 2));
}

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
