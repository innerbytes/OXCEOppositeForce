#include <type_traits>

#include "Battlescape/BattlescapeGame.h"
#include "Battlescape/BattlescapeState.h"
#include "Engine/Options.h"
#include "Fixtures.h"
#include "Mod/ModScript.h"
#include "OppositeForce/Battlescape/Game/MissionCompletion.h"
#include "Savegame/BattleUnit.h"
#include "Savegame/SavedBattleGame.h"
#include "catch_amalgamated.hpp"

using namespace OpenXcom;
using namespace OpenXcom::OppositeForceTests;

TEST_CASE("MissionCompletion_DefaultState_PreservesNativeResults", "[mission-completion]")
{
    // Arrange
    MissionCompletion completion;

    // Act
    const auto result = completion.getResult();

    // Assert
    REQUIRE_FALSE(completion.isManualCompletion());
    REQUIRE(completion.getResult() == MissionCompletion::Result::None);
    REQUIRE(result == MissionCompletion::Result::None);
}

TEST_CASE("MissionCompletion_ManualModeToggle_RestoresNativeChecksWithoutClearingRequest", "[mission-completion]")
{
    // Arrange
    const auto pendingResult = GENERATE(MissionCompletion::Result::None, MissionCompletion::Result::Success,
                                        MissionCompletion::Result::Failure);
    MissionCompletion completion;
    completion.setManualCompletion(true);
    completion.setResult(pendingResult);

    // Act
    completion.setManualCompletion(false);

    // Assert
    REQUIRE_FALSE(completion.isManualCompletion());
    REQUIRE(completion.getResult() == pendingResult);
}

TEST_CASE("MissionCompletion_SetResult_ReplacesOrClearsPendingResultInEitherMode", "[mission-completion]")
{
    // Arrange
    const bool manual = GENERATE(false, true);
    const auto requestedResult = GENERATE(MissionCompletion::Result::None, MissionCompletion::Result::Success,
                                          MissionCompletion::Result::Failure);
    MissionCompletion completion;
    completion.setManualCompletion(manual);
    completion.setResult(MissionCompletion::Result::Success);

    // Act
    completion.setResult(requestedResult);
    const auto result = completion.getResult();
    completion.setResult(MissionCompletion::Result::None);

    // Assert
    REQUIRE(completion.isManualCompletion() == manual);
    REQUIRE(result == requestedResult);
    REQUIRE(completion.getResult() == MissionCompletion::Result::None);
}

TEST_CASE("MissionCompletion_Reset_RestoresDefaultState", "[mission-completion]")
{
    // Arrange
    const bool success = GENERATE(false, true);
    MissionCompletion completion;
    completion.setManualCompletion(true);
    completion.setResult(success ? MissionCompletion::Result::Success : MissionCompletion::Result::Failure);

    // Act
    completion.reset();

    // Assert
    REQUIRE_FALSE(completion.isManualCompletion());
    REQUIRE(completion.getResult() == MissionCompletion::Result::None);
}
