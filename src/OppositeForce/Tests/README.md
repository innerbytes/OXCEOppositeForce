# Opposite Force headless tests

Run from the repository root:

```sh
pnpm test
```

The runner configures a separate Release build and runs Catch2 tests without
launching the game. No installed mod or user save is required. Catch2 is vendored;
no framework installation or download is needed.

## Requirements

All platforms need Node.js 18+, pnpm, CMake 3.19+, a C++17 compiler, a build tool,
and the engine's development dependencies: SDL 1.2, SDL_image, SDL_gfx, SDL_mixer,
zlib, and OpenGL when enabled.

| Platform | Setup |
| --- | --- |
| macOS | Clang and make. The runner uses the active Xcode or Command Line Tools SDK. The default dependency prefix is `/opt/local` (MacPorts). |
| Linux | GCC or Clang, make, and pkg-config. Install the engine's development dependencies or configure their paths. |
| Windows | Ninja and `clang-cl` from a Visual Studio Native Tools prompt or Developer PowerShell, with C++ tools, Windows SDK, and Clang installed. Supply engine dependencies in the existing `deps` layout or set `DEPS_DIR`. Visual Studio generators use `ClangCL`; plain `cl.exe` is unsupported by the fixtures. |

## Running selected tests

Catch2 filters and reporting options are forwarded after `--`:

```sh
pnpm test -- '[hidden-ufo]'
pnpm test -- '[hangars]'
pnpm test -- '[funding]'
pnpm test -- '[avatar]'
pnpm test -- --list-tests
pnpm test -- --order rand --rng-seed 12345
pnpm test -- --reporter junit --out src/OppositeForce/Tests/build/results.xml
```

## Configuration

The runner respects `CMAKE_GENERATOR`, `CMAKE_GENERATOR_PLATFORM`,
`CMAKE_GENERATOR_TOOLSET`, `CC`, `CXX`, `CMAKE_PREFIX_PATH`, and
`CMAKE_TOOLCHAIN_FILE`. On macOS, `SDKROOT` selects the SDK.

Set `OF_TEST_CMAKE_ARGS` to a JSON array for additional CMake arguments:

```sh
# Linux/macOS
export CMAKE_GENERATOR=Ninja
export OF_TEST_CMAKE_ARGS='["-DDEPS_DIR=/path/to/dependencies"]'
pnpm test
```

```powershell
# Windows Developer PowerShell
$env:OF_TEST_CMAKE_ARGS = '["-DDEPS_DIR=C:/path/to/dependencies"]'
pnpm test

# Alternatively, use a Visual Studio generator on a fresh build.
$env:CMAKE_GENERATOR = 'Visual Studio 17 2022'
$env:CMAKE_GENERATOR_PLATFORM = 'x64'
pnpm test
```

Additional arguments cannot override the required Release configuration, test
registration hook, or disabled packaging, embedded assets, and crash handler.
The runner adds configured dependency directories to the test process's Windows
`Path` or Linux `LD_LIBRARY_PATH`.

Build output is stored in `src/OppositeForce/Tests/build/`, independently of the
engine's root `build/`. Subsequent runs reuse it. Delete the test build directory
to start fresh or before changing the generator, compiler, or toolchain.
