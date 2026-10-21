#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Engine/Language.h"
#include "Engine/RNG.h"
#include "Mod/AlienDeployment.h"
#include "Mod/Armor.h"
#include "Mod/MapDataSet.h"
#include "Mod/Mod.h"
#include "Mod/RuleAlienMission.h"
#include "Mod/RuleBaseFacility.h"
#include "Mod/RuleCountry.h"
#include "Mod/RuleCraft.h"
#include "Mod/RuleItem.h"
#include "Mod/RuleManufacture.h"
#include "Mod/RuleRegion.h"
#include "Mod/RuleSoldier.h"
#include "Mod/RuleTerrain.h"
#include "Mod/RuleUfo.h"
#include "Mod/Unit.h"
#include "OppositeForce/Savegame/HangarCompatibility.h"
#include "Savegame/AlienMission.h"
#include "Savegame/Base.h"
#include "Savegame/BaseFacility.h"
#include "Savegame/Craft.h"
#include "Savegame/Production.h"
#include "Savegame/SavedGame.h"
#include "Savegame/Soldier.h"
#include "Savegame/Transfer.h"
#include "Savegame/Ufo.h"

namespace OpenXcom::OppositeForceTests
{
    class RulesFixture
    {
    public:
        RulesFixture();
        ~RulesFixture();
        Mod &getMod();
        ScriptGlobal *getShared();
        RuleUfo &createUfo(const std::string &type, const std::string &yaml = "{}");
        RuleCraft &createCraft(const std::string &type, const std::string &yaml = "{}");
        RuleBaseFacility &createFacility(const std::string &type, const std::string &yaml);
        RuleCountry &createCountry(const std::string &type, const std::string &yaml = "{}");
        RuleSoldier &createSoldier(const std::string &type, const std::string &yaml = "{}");
        RuleManufacture &createManufacture(const std::string &type, const std::string &craftType);
        RuleAlienMission &createAlienMission(const std::string &type, const std::string &yaml);
        AlienDeployment &createDeployment(const std::string &type, const std::string &yaml = "{}");
        const std::vector<MapScript *> &createMapScript(const std::string &type, const std::string &yaml);
        Armor &createArmor(const std::string &type);
        Unit &createUnit(const std::string &type, const std::string &yaml = "{}");
        RuleItem &createItem(const std::string &type, const std::string &yaml = "{}");
        RuleTerrain &createTerrain(const std::string &type, const std::string &yaml);
        MapDataSet &createMapDataSet(const std::string &type);
        void createInventoryGround();
        void loadRule(RuleUfo &rule, const std::string &yaml);
        void loadRule(RuleCraft &rule, const std::string &yaml);
        void loadRule(RuleBaseFacility &rule, const std::string &yaml);
        void loadRule(RuleCountry &rule, const std::string &yaml);
        void loadRule(RuleSoldier &rule, const std::string &yaml);
        void loadRule(AlienDeployment &rule, const std::string &yaml);
        void loadRule(Unit &rule, const std::string &yaml);
        void loadRule(Armor &rule, const std::string &yaml);
        void setDestroyedFacility(RuleBaseFacility &rule, const RuleBaseFacility &replacement);
        MissionSite *spawnMissionSite(AlienMission &mission, SavedGame &save, const MissionArea &area, const Ufo *ufo);

    private:
        struct Implementation;
        uint64_t _previousSeed;
        std::unique_ptr<Implementation> _implementation;
    };

    class BaseFixture
    {
    public:
        BaseFixture();
        ~BaseFixture();
        RulesFixture rules;
        Base &getBase();
        BaseFacility &addFacility(const RuleBaseFacility &rule, int buildTime = 0);
        Craft &addCraft(const RuleCraft &rule, const std::string &status = "STR_READY");
        Transfer &addTransfer(const RuleCraft &rule);
        Production &addProduction(const RuleCraft &rule, int total, int produced = 0);
        void clearRequests();

    private:
        std::unique_ptr<Base> _base;
    };

    class DetectionFixture
    {
    public:
        explicit DetectionFixture(const std::string &ufoYaml);
        BaseFixture base;
        SavedGame save;
        Ufo &getUfo();
        Craft &getCraft();
        void addRadar(bool hyperwave);

    private:
        std::unique_ptr<Ufo> _ufo;
        Craft *_craft;
    };

    std::string getForcedDetectionScripts();
    void loadLanguage(Language &language, const std::string &yaml);
    TransferRow createPendingRow(TransferType type, const void *rule, int amount);

    struct MatchingScenario
    {
        std::vector<std::string> hangarTypes;
        std::vector<std::vector<std::string>> restrictions;
        std::vector<std::string> candidateTypes;
    };

    MatchingScenario createMatchingScenario(int seed);
    RuleCraft &addMatchingScenario(BaseFixture &fixture, const MatchingScenario &scenario);
    int calculateExhaustiveFreeSlots(const MatchingScenario &scenario);
}  // namespace OpenXcom::OppositeForceTests
