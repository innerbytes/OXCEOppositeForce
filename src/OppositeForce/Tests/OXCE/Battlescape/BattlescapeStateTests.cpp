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
using namespace OppositeForce;
using namespace OppositeForce::Tests;

TEST_CASE("BattlescapeState_NativeFinishBattle_PreservesUpstreamSignature", "[mission-completion][integration]")
{
    // Arrange
    using NativeFinishSignature = void (BattlescapeState::*)(bool, int);

    // Act
    constexpr bool matchesNativeSignature =
        std::is_same_v<decltype(&BattlescapeState::finishBattle), NativeFinishSignature>;

    // Assert
    STATIC_REQUIRE(matchesNativeSignature);
}
