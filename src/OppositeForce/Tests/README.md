# Opposite Force headless tests

Run `pnpm test` from the repository root. It configures an independent Release
build, builds only `opposite_force_tests`, and runs the vendored Catch2 runner.
The suite exercises the current fork with real engine methods and in-memory YAML.
It needs no installed mod, user save, SDL window, game launch, or debugger.

## Requirements and commands

The helper currently supports macOS, using the same dependencies as `pnpm build`:
Node.js 18 or newer, pnpm, CMake 3.19 or newer, make, Clang, a macOS SDK with C++
headers, and the engine's existing MacPorts dependencies under `/opt/local`.
`SDKROOT` can select an SDK; otherwise the helper checks the active Xcode SDK and
then the Command Line Tools SDK. Catch2 itself needs no installation or download.

Catch2 arguments are forwarded unchanged, with an optional `--` separator:

```sh
pnpm test
pnpm test -- '[hidden-ufo]'
pnpm test -- '[hangars]'
pnpm test -- '[funding]'
pnpm test -- '[avatar]'
pnpm test -- --list-tests
pnpm test -- --order rand --rng-seed 12345
pnpm test -- --reporter junit --out src/OppositeForce/Tests/build/results.xml
```

Build products and reports belong in the ignored `build/` directory here. Delete
that directory to start fresh. The ordinary root `build/` directory is independent.
The helper prints configure, build, and test timings separately.

## Isolation and fixtures

`RegisterTests.cmake` is injected only into the test build using
`CMAKE_PROJECT_OpenXcom_INCLUDE`. Deferred registration includes this directory's
`CMakeLists.txt` after the existing `openxcom` target has been defined. A static
test engine library derives its sources, compile settings, source-specific flags,
includes, and dependencies from that target. Application entry points and
non-C/C++ resources are excluded. Existing CMake files are unchanged.

`EngineEntryPoint.cpp` includes the unchanged `main.cpp` with its entry point
renamed. This retains the real `Exception` constructor and engine globals; the
renamed entry point is never called. Catch2 supplies the test entry point,
discovery, generators, assertions, filtering, and reporters.

Only `Fixtures.cpp` receives `-fno-access-control`. It constructs the real private
parser set, registers minimally configured rules in a real `Mod`, connects a
destroyed-facility rule without asset validation, and calls the real private
`AlienMission::spawnMissionSite` method. Engine sources and test cases retain
ordinary access checks. No production method is mocked or reimplemented.

Fixtures own their engine objects and rules. Each fixture sets a fixed engine RNG
seed and restores the previous seed. Base cleanup also clears the production
hangar-warning cache through its normal public API. The Catch2 listener enables
YAML's exception handler and temporarily mutes audio, restoring the previous mute
option at the end of the run. No SDL or audio device is initialized.

The only repository infrastructure outside this directory is
`scripts-of/test.js` and the approved `package.json` test command. Neither test
configuration nor execution reads a pre-fork commit or builds an upstream baseline.

## Coverage

| Feature | Production behavior exercised |
| --- | --- |
| Hangar rules | Optional defaults, rule layering and inheritance, case-sensitive matching, unrestricted pairings, arbitrary and duplicate types, invalid scalar/empty lists/strings, and typed facilities requiring effective capacity one. |
| Hangar allocation | Completed and unfinished facilities, resident and airborne craft, transfers, both pending-row representations, ignored non-craft rows, remaining production reservations, full capacity, and reassignment across competing restrictions. Sixty-four deterministic small scenarios compare the engine result with an independent oracle that enumerates slot permutations, with at most five slots. |
| Hangar lifecycle | Removal and replacement checks, previous capacity during upgrades, alphabetical display preference and invalid-assignment fallback, damage/destruction preparation, compatible replacements, warning suppression and reset, and real `Base::isAreaInUse`, `Base::destroyFacility`, and `Base::damageFacility` integration. |
| Funding names | Default/overridden keys, layering and inheritance, empty fallback, unknown country IDs, unchanged identity/geography/funding, translated singular/plural reports, mixed lists, separators, and unchanged text when overrides are absent. |
| Avatar restriction | Default/false/true/inherited/overridden values, rule access through real soldiers, and unchanged existing and saved appearance. |
| Hidden UFOs | Rule defaults/loading/layering, base radar and hyperwave, guaranteed craft radar, tracked targets, force-detection scripts, unchanged engine RNG during suppressed detection, protected setters, saved flags, crash reveal suppression, explicit debug reveal/reload/cleanup, markers, movement, landing countdown, and hunting target assignment. Visible UFOs serve as normal-behavior controls. |
| Hidden-UFO sites | Real site spawning with a hidden UFO: saved-game registration, coordinates, deployment, lifetime, and independent site detection and marker behavior. |

The former `build/of-hidden-ufo-check.cpp` had 44 executed checks, including loop
iterations. All are represented by named cases and generator rows in
`HiddenUfoTests.cpp`; the old temporary harness is not a suite dependency:

| Previous checks | Permanent cases / theories |
| --- | --- |
| 1–7: rule defaults, omission, explicit values, layering, inheritance | `RuleUfo_DefaultConstruction_IsVisible`, `RuleUfo_OptionalHiddenValues_LoadsExpectedVisibility`, `RuleUfo_LayeredHiddenValues_PreservesOrOverridesPreviousValue` |
| 8–10: protected flags, hidden marker, normal setters | `Ufo_HiddenUfoDuringNormalPlay_HasNoDetectionFlagsOrMarker`, `Ufo_NormalSettersOnVisibleUfo_StoresDetectionFlags` |
| 11–23: base/craft detection, tracking, hyperwave, unchanged RNG, visible controls | `BaseDetect_HiddenUfo_ReturnsNoneWithoutConsumingRandomness`, `CraftDetect_HiddenUfoWithGuaranteedRadar_ReturnsNoneWithoutConsumingRandomness`, `BaseDetect_VisibleUfoWithRadar_ReturnsConfiguredDetectionType`, `CraftDetect_VisibleUfoWithGuaranteedRadar_ReturnsRadarDetection` |
| 24–31: forced scripts with tracked/untracked hidden and visible UFOs | `BaseDetect_ForcedDetectionScript_RespectsHiddenRestriction`, `CraftDetect_ForcedDetectionScript_RespectsHiddenRestriction` |
| 32–33: hidden/visible crash reveals | `Ufo_CrashedStatus_RevealsOnlyVisibleRules` |
| 34–35: debug reveal and crash preservation | `Ufo_DebugRevealOnHiddenUfo_SetsBothFlagsAndShowsMarker`, `Ufo_CrashedAfterDebugReveal_PreservesBothDetectionFlags` |
| 36–37: hidden/visible saved detection flags | `Ufo_ReloadAfterDebugReveal_ClearsFlagsOnlyForHiddenRules` |
| 38–39: true setters clear the corresponding debug flag | `Ufo_SetDetectedOnHiddenUfo_ClearsRadarFlagAndPreservesHyperwaveFlag`, `Ufo_SetHyperDetectedOnHiddenUfo_ClearsHyperwaveFlagAndPreservesRadarFlag` |
| 40: lifecycle cleanup | `Ufo_CleanupAfterDebugReveal_ClearsBothFlagsAndMarker` |
| 41–42: landing countdown and movement | `Ufo_LandedHiddenUfo_AdvancesLandingCountdown`, `Ufo_FlyingHiddenUfo_MovesTowardDestination` |
| 43–44: hunting assignment and reveal protection | `Ufo_HiddenUfoReceivesHuntingTarget_KeepsTargetWithoutRevealing` |

### Remaining gaps

Avatar UI interaction, branding display, Geoscape detection alerts and sounds,
hunter-killer notifications, debug keyboard handling, tracker/globe rendering, and
the complete mission-wave/trajectory/land-check/site-alert flow require game state
or assets that these fixtures do not initialize. The site tests call the spawning
method directly; they do not claim to test land-check fallback or
`objectiveOnTheLandingSite` orchestration. Marker values and hunting assignment are
covered, while their UI callers are not.

## Adding tests

Add test sources here and register them in the test-local `CMakeLists.txt`. Use
`EntityUnderTest_Condition_ExpectedResult` names and tags for the feature. Every
test body has the exact `// Arrange`, `// Act`, and `// Assert` comments with blank
lines between phases. Keep assertions in the test and setup/ownership in fixtures.
Use Catch2 generators/tables when rows share an operation and assertions, without
branching inside test bodies. Keep exception theories separate: define the
operation in Act and invoke it through an exception assertion in Assert.

Format authored C++ through `pnpm prettier` with explicit file arguments. Vendored
Catch2 and generated build sources have local `DisableFormat: true` settings so
the existing recursive formatter preserves them.

## Framework provenance

`vendor/Catch2` contains the unmodified official amalgamation from
[Catch2 v3.16.0](https://github.com/catchorg/Catch2/releases/tag/v3.16.0):

- [catch_amalgamated.hpp](https://raw.githubusercontent.com/catchorg/Catch2/v3.16.0/extras/catch_amalgamated.hpp)
- [catch_amalgamated.cpp](https://raw.githubusercontent.com/catchorg/Catch2/v3.16.0/extras/catch_amalgamated.cpp)
- [LICENSE.txt](https://raw.githubusercontent.com/catchorg/Catch2/v3.16.0/LICENSE.txt)

The original copyright notices and Boost Software License 1.0 are preserved. The
framework remains under that license; the repository's GPLv3 license is unchanged.
The [GNU license list](https://www.gnu.org/licenses/license-list.html#boost)
identifies the Boost Software License as GPL-compatible. To update the
framework, replace all three files together from an official release, preserve
notices, and rerun the suite; normal test runs do not fetch anything.

SHA-256 of the checked-in upstream files:

```text
d4cc143ea76ae212204363922d8adf376d66a1fda5a33ac73f93a7d1c119f4e0  catch_amalgamated.hpp
1fe7f10334e0ae5494419cfa84c270f15235eada0fc02bdb493d1def537601f3  catch_amalgamated.cpp
c9bff75738922193e67fa726fa225535870d2aa1059f91452c411736284ad566  LICENSE.txt
```

## Validation record

Validation on this macOS workspace uses 75 named cases and 347 assertions,
including generator rows. Both ordinary and shuffled runs pass. `pnpm build`
also succeeds without launching the game. All authored C++ was formatted with
`pnpm prettier`; every test was reviewed for naming, phase comments, and
branch-free theory bodies. The final diff contains only this directory, the helper,
and one `package.json` script entry.

| Measurement | Wall time |
| --- | ---: |
| Full clean compilation of the test target, two jobs, existing CMake cache | 241.42 s |
| Warm `pnpm test`: configure | 0.24 s |
| Warm `pnpm test`: incremental build | 2.45 s |
| Warm `pnpm test`: test execution | 4.28 s |
| Warm `pnpm test`: entire command including pnpm/Node startup | 7.15 s |
| Shuffled test execution, `--order rand --rng-seed 12345` | 4.91 s |

These timings are observations from this workspace, not performance assertions.
The first configuration adds a small configure step to the clean compilation;
subsequent commands reuse the test-local engine build. Filtered execution and
Catch2 JUnit reporting are also checked through `pnpm test`.
