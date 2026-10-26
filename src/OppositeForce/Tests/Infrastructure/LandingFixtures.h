#pragma once

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
#include "OppositeForce/Battlescape/Game/GlobalTerrain.h"
#include "OppositeForce/Geoscape/Game/LandingTextures.h"
#include "Savegame/MissionSite.h"

namespace OppositeForce::Tests
{
    using namespace OpenXcom;

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
                std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path().parent_path();
            Options::setDataFolder((repository / "bin").string() + "/");
            FileMap::clear(false, false);
        }

        ~HeadlessSettings()
        {
            FileMap::clear(true, false);
            for (auto &restore : _restore)
            {
                restore();
            }
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
            for (auto *ufo : *save.getUfos())
            {
                delete ufo;
            }
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
}  // namespace OppositeForce::Tests
