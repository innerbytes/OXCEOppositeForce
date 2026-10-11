#include "Fixtures.h"

#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>

#include "Mod/ExtraStrings.h"
#include "Mod/ModScript.h"

namespace OpenXcom::OppositeForceTests
{
    struct RulesFixture::Implementation
    {
        Mod mod;
        std::unique_ptr<ModScript> parsers;

        ModScript &getParsers()
        {
            if (!parsers)
            {
                mod.getScriptGlobal()->beginLoad();
                parsers.reset(new ModScript(mod.getScriptGlobal(), &mod));
            }
            return *parsers;
        }
    };

    RulesFixture::RulesFixture() : _previousSeed(RNG::getSeed()), _implementation(std::make_unique<Implementation>())
    {
        RNG::setSeed(123456789);
    }

    RulesFixture::~RulesFixture()
    {
        RNG::setSeed(_previousSeed);
    }

    Mod &RulesFixture::getMod()
    {
        return _implementation->mod;
    }

    ScriptGlobal *RulesFixture::getShared()
    {
        return getMod().getScriptGlobal();
    }

    template <typename Rule>
    static Rule &registerRule(std::map<std::string, Rule *> &registry, const std::string &type,
                              std::unique_ptr<Rule> rule)
    {
        if (!registry.emplace(type, rule.get()).second)
        {
            throw std::logic_error("Duplicate fixture rule: " + type);
        }
        return *rule.release();
    }

    RuleUfo &RulesFixture::createUfo(const std::string &type, const std::string &yaml)
    {
        auto rule = std::make_unique<RuleUfo>(type);
        loadRule(*rule, yaml);
        return registerRule(getMod()._ufos, type, std::move(rule));
    }

    RuleCraft &RulesFixture::createCraft(const std::string &type, const std::string &yaml)
    {
        auto rule = std::make_unique<RuleCraft>(type, 0);
        loadRule(*rule, yaml);
        return registerRule(getMod()._crafts, type, std::move(rule));
    }

    RuleBaseFacility &RulesFixture::createFacility(const std::string &type, const std::string &yaml)
    {
        auto rule = std::make_unique<RuleBaseFacility>(type, 0);
        loadRule(*rule, yaml);
        return registerRule(getMod()._facilities, type, std::move(rule));
    }

    RuleCountry &RulesFixture::createCountry(const std::string &type, const std::string &yaml)
    {
        auto rule = std::make_unique<RuleCountry>(type);
        loadRule(*rule, yaml);
        return registerRule(getMod()._countries, type, std::move(rule));
    }

    RuleSoldier &RulesFixture::createSoldier(const std::string &type, const std::string &yaml)
    {
        auto rule = std::make_unique<RuleSoldier>(type, 0);
        loadRule(*rule, yaml);
        return registerRule(getMod()._soldiers, type, std::move(rule));
    }

    RuleManufacture &RulesFixture::createManufacture(const std::string &type, const std::string &craftType)
    {
        auto rule = std::make_unique<RuleManufacture>(type, 0);
        YAML::YamlRootNodeReader reader(
            YAML::YamlString("category: STR_CRAFT\ntime: 10\nproducedItems:\n  " + craftType + ": 1\n"),
            "test manufacture");
        rule->load(reader, &getMod());
        rule->afterLoad(&getMod());
        return registerRule(getMod()._manufacture, type, std::move(rule));
    }

    RuleAlienMission &RulesFixture::createAlienMission(const std::string &type, const std::string &yaml)
    {
        auto rule = std::make_unique<RuleAlienMission>(type);
        YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "test mission rule");
        rule->load(reader);
        return registerRule(getMod()._alienMissions, type, std::move(rule));
    }

    AlienDeployment &RulesFixture::createDeployment(const std::string &type, const std::string &yaml)
    {
        auto rule = std::make_unique<AlienDeployment>(type);
        YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "test deployment");
        rule->load(reader, &getMod());
        return registerRule(getMod()._alienDeployments, type, std::move(rule));
    }

    Armor &RulesFixture::createArmor(const std::string &type)
    {
        auto rule = std::make_unique<Armor>(type, 0);
        return registerRule(getMod()._armors, type, std::move(rule));
    }

    void RulesFixture::loadRule(RuleUfo &rule, const std::string &yaml)
    {
        YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "test UFO rule");
        rule.load(reader, &getMod(), _implementation->getParsers());
    }

    void RulesFixture::loadRule(RuleCraft &rule, const std::string &yaml)
    {
        YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "test craft rule");
        rule.load(reader, &getMod(), _implementation->getParsers());
    }

    void RulesFixture::loadRule(RuleBaseFacility &rule, const std::string &yaml)
    {
        YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "test facility rule");
        rule.load(reader, &getMod());
    }

    void RulesFixture::loadRule(RuleCountry &rule, const std::string &yaml)
    {
        YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "test country rule");
        rule.load(reader, _implementation->getParsers(), &getMod());
    }

    void RulesFixture::loadRule(RuleSoldier &rule, const std::string &yaml)
    {
        YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "test soldier rule");
        rule.load(reader, &getMod(), _implementation->getParsers());
    }

    void RulesFixture::setDestroyedFacility(RuleBaseFacility &rule, const RuleBaseFacility &replacement)
    {
        // Linking this fixture reference directly avoids unrelated sprite/resource validation.
        rule._destroyedFacility = &replacement;
    }

    MissionSite *RulesFixture::spawnMissionSite(AlienMission &mission, SavedGame &save, const MissionArea &area,
                                                const Ufo *ufo)
    {
        // Invoke the real resource-free spawning method; its normal caller requires Game and Globe UI objects.
        return mission.spawnMissionSite(save, getMod(), area, ufo);
    }

    BaseFixture::BaseFixture() : _base(std::make_unique<Base>(&rules.getMod())) {}

    BaseFixture::~BaseFixture()
    {
        clearRequests();
        // The production warning cache is reset by its public API when a base becomes valid.
        HangarCompatibility::shouldWarn(*_base);
    }

    Base &BaseFixture::getBase()
    {
        return *_base;
    }

    BaseFacility &BaseFixture::addFacility(const RuleBaseFacility &rule, int buildTime)
    {
        auto *facility = new BaseFacility(&rule, _base.get());
        facility->setX(static_cast<int>(_base->getFacilities()->size()));
        facility->setBuildTime(buildTime);
        _base->getFacilities()->push_back(facility);
        return *facility;
    }

    Craft &BaseFixture::addCraft(const RuleCraft &rule, const std::string &status)
    {
        auto *craft = new Craft(&rule, _base.get(), static_cast<int>(_base->getCrafts()->size()) + 1);
        craft->setStatus(status);
        _base->getCrafts()->push_back(craft);
        return *craft;
    }

    Transfer &BaseFixture::addTransfer(const RuleCraft &rule)
    {
        auto *transfer = new Transfer(24);
        transfer->setCraft(new Craft(&rule, _base.get(), 100));
        _base->getTransfers()->push_back(transfer);
        return *transfer;
    }

    Production &BaseFixture::addProduction(const RuleCraft &rule, int total, int produced)
    {
        auto &manufacture =
            rules.createManufacture("PRODUCTION_" + std::to_string(_base->getProductions().size()), rule.getType());
        auto *production = new Production(&manufacture, total);
        production->setTimeSpent(produced * manufacture.getManufactureTime());
        _base->addProduction(production);
        return *production;
    }

    void BaseFixture::clearRequests()
    {
        for (auto *craft : *_base->getCrafts()) delete craft;
        _base->getCrafts()->clear();
        for (auto *transfer : *_base->getTransfers()) delete transfer;
        _base->getTransfers()->clear();
        while (!_base->getProductions().empty()) _base->removeProduction(_base->getProductions().back());
    }

    DetectionFixture::DetectionFixture(const std::string &ufoYaml)
        : _ufo(std::make_unique<Ufo>(&base.rules.createUfo("UFO", ufoYaml), 1)),
          _craft(&base.addCraft(base.rules.createCraft("RADAR_CRAFT")))
    {
    }

    Ufo &DetectionFixture::getUfo()
    {
        return *_ufo;
    }

    Craft &DetectionFixture::getCraft()
    {
        return *_craft;
    }

    void DetectionFixture::addRadar(bool hyperwave)
    {
        auto &rule = base.rules.createFacility(
            "RADAR", "radarRange: 1000\nradarChance: 100\nhyper: " + std::string(hyperwave ? "true" : "false"));
        base.addFacility(rule);
    }

    std::string getForcedDetectionScripts()
    {
        return "scripts:\n"
               "  detectUfoFromBase: |\n"
               "    set detection_type 3;\n"
               "    set detection_chance 100;\n"
               "    return detection_type detection_chance;\n"
               "  detectUfoFromCraft: |\n"
               "    set detection_type 3;\n"
               "    set detection_chance 100;\n"
               "    return detection_type detection_chance;\n";
    }

    void loadLanguage(Language &language, const std::string &yaml)
    {
        ExtraStrings strings;
        YAML::YamlRootNodeReader reader(YAML::YamlString(yaml), "test language");
        strings.load(reader);
        language.loadRule({{"en-US", &strings}}, "en-US");
    }

    TransferRow createPendingRow(TransferType type, const void *rule, int amount)
    {
        TransferRow row{};
        row.type = type;
        row.rule = rule;
        row.amount = amount;
        return row;
    }

    static std::vector<std::string> getTypesFromMask(unsigned mask)
    {
        std::vector<std::string> types;
        for (unsigned bit = 0; bit < 3; ++bit)
        {
            if (mask & (1u << bit)) types.push_back(std::string(1, 'A' + bit));
        }
        return types;
    }

    MatchingScenario createMatchingScenario(int seed)
    {
        std::mt19937 random(static_cast<unsigned>(seed));
        MatchingScenario scenario;
        const auto slots = random() % 5 + 1;
        for (unsigned i = 0; i < slots; ++i)
        {
            const auto type = random() % 4;
            scenario.hangarTypes.push_back(type == 3 ? "" : std::string(1, 'A' + type));
        }
        const auto requests = random() % (slots + 2);
        for (unsigned i = 0; i < requests; ++i) scenario.restrictions.push_back(getTypesFromMask(random() % 8));
        scenario.candidateTypes = getTypesFromMask(random() % 7 + 1);
        return scenario;
    }

    static std::string getCraftYaml(const std::vector<std::string> &types)
    {
        if (types.empty()) return "{}";
        std::string yaml = "allowedHangarTypes:\n";
        for (const auto &type : types) yaml += "  - " + type + "\n";
        return yaml;
    }

    RuleCraft &addMatchingScenario(BaseFixture &fixture, const MatchingScenario &scenario)
    {
        for (size_t index = 0; index < scenario.hangarTypes.size(); ++index)
        {
            const auto &type = scenario.hangarTypes[index];
            const auto yaml = "crafts: 1\n" + (type.empty() ? "" : "hangarType: " + type + "\n");
            fixture.addFacility(fixture.rules.createFacility("SLOT_" + std::to_string(index), yaml));
        }
        for (size_t index = 0; index < scenario.restrictions.size(); ++index)
        {
            fixture.addCraft(fixture.rules.createCraft("CRAFT_" + std::to_string(index),
                                                       getCraftYaml(scenario.restrictions[index])));
        }
        return fixture.rules.createCraft("CANDIDATE", getCraftYaml(scenario.candidateTypes));
    }

    static bool canAssignExhaustively(const MatchingScenario &scenario, size_t candidateCount)
    {
        auto requests = scenario.restrictions;
        requests.insert(requests.end(), candidateCount, scenario.candidateTypes);
        if (requests.size() > scenario.hangarTypes.size()) return false;
        std::vector<size_t> slots(scenario.hangarTypes.size());
        std::iota(slots.begin(), slots.end(), 0);
        // Enumerate every slot permutation, independently of the production augmenting-path algorithm.
        do
        {
            bool compatible = true;
            for (size_t index = 0; index < requests.size(); ++index)
            {
                const auto &type = scenario.hangarTypes[slots[index]];
                const auto &allowed = requests[index];
                if (!type.empty() && !allowed.empty() &&
                    std::find(allowed.begin(), allowed.end(), type) == allowed.end())
                {
                    compatible = false;
                    break;
                }
            }
            if (compatible) return true;
        } while (std::next_permutation(slots.begin(), slots.end()));
        return false;
    }

    int calculateExhaustiveFreeSlots(const MatchingScenario &scenario)
    {
        if (!canAssignExhaustively(scenario, 0)) return 0;
        int free = 0;
        for (size_t count = 1; count <= scenario.hangarTypes.size(); ++count)
        {
            if (!canAssignExhaustively(scenario, count)) break;
            ++free;
        }
        return free;
    }
}  // namespace OpenXcom::OppositeForceTests
