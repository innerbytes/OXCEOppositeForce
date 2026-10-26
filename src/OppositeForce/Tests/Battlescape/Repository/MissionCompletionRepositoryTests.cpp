#include <type_traits>

#include "Battlescape/BattlescapeGame.h"
#include "Battlescape/BattlescapeState.h"
#include "Engine/Options.h"
#include "Fixtures.h"
#include "Mod/ModScript.h"
#include "OppositeForce/Battlescape/Game/MissionCompletion.h"
#include "OppositeForce/Battlescape/Repository/MissionCompletionRepository.h"
#include "Savegame/BattleUnit.h"
#include "Savegame/SavedBattleGame.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

TEST_CASE("MissionCompletion_SaveRoundTrip_PreservesPendingResult", "[mission-completion][save]")
{
    // Arrange
    const bool success = GENERATE(false, true);
    MissionCompletion original;
    original.setManualCompletion(true);
    original.setResult(success ? MissionCompletion::Result::Success : MissionCompletion::Result::Failure);
    YAML::YamlRootNodeWriter snapshot;
    MissionCompletionRepository::save(original, snapshot);
    MissionCompletion reloaded;

    // Act
    MissionCompletionRepository::load(reloaded, snapshot.toReader());
    const auto result = reloaded.getResult();

    // Assert
    REQUIRE(reloaded.isManualCompletion());
    REQUIRE(result == (success ? MissionCompletion::Result::Success : MissionCompletion::Result::Failure));
    REQUIRE(reloaded.getResult() == result);
    REQUIRE(snapshot.toReader()["ofMissionCompletion"]["isManualCompletion"].readVal<bool>());
    REQUIRE(snapshot.toReader()["ofMissionCompletion"]["pendingResult"].readVal<int>() == (success ? 1 : 2));
    REQUIRE_FALSE(snapshot.toReader()["isManualCompletion"]);
    REQUIRE_FALSE(snapshot.toReader()["pendingResult"]);
}

TEST_CASE("MissionCompletion_SavedKeys_LoadsManualModeAndPendingResult", "[mission-completion][save]")
{
    // Arrange
    const bool manual = GENERATE(false, true);
    const auto [pendingResult, expectedResult] = GENERATE(table<int, MissionCompletion::Result>({
        {-1, MissionCompletion::Result::None},
        {0, MissionCompletion::Result::None},
        {1, MissionCompletion::Result::Success},
        {2, MissionCompletion::Result::Failure},
    }));
    YAML::YamlRootNodeReader snapshot(
        YAML::YamlString("ofMissionCompletion: {isManualCompletion: " + std::string(manual ? "true" : "false") +
                         ", pendingResult: " + std::to_string(pendingResult) + "}"),
        "mission completion save");
    MissionCompletion completion;
    completion.setManualCompletion(!manual);
    completion.setResult(MissionCompletion::Result::Success);

    // Act
    MissionCompletionRepository::load(completion, snapshot);
    const auto result = completion.getResult();

    // Assert
    REQUIRE(completion.isManualCompletion() == manual);
    REQUIRE(result == expectedResult);
}

TEST_CASE("MissionCompletion_OldSaveWithoutFields_ResetsToNativeBehavior", "[mission-completion][save]")
{
    // Arrange
    MissionCompletion completion;
    completion.setManualCompletion(true);
    completion.setResult(MissionCompletion::Result::Success);
    YAML::YamlRootNodeReader oldSave(YAML::YamlString("turn: 1"), "old battle save");

    // Act
    MissionCompletionRepository::load(completion, oldSave);

    // Assert
    REQUIRE_FALSE(completion.isManualCompletion());
    REQUIRE(completion.getResult() == MissionCompletion::Result::None);
}
