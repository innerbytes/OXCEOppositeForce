import { spawnSync } from "node:child_process";
import { existsSync, mkdtempSync, readFileSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";

const repositoryRoot = fileURLToPath(new URL("../", import.meta.url));
const libraries = [
  { name: "SDL 1.2", checks: ["sdl >= 1.2", "sdl < 2"], port: "libsdl", apt: "libsdl1.2-dev" },
  { name: "SDL_image 1.2", checks: ["SDL_image"], port: "libsdl_image", apt: "libsdl-image1.2-dev" },
  { name: "SDL_mixer 1.2", checks: ["SDL_mixer"], port: "libsdl_mixer", apt: "libsdl-mixer1.2-dev" },
  { name: "SDL_gfx >= 2.0.22", checks: ["SDL_gfx >= 2.0.22"], port: "libsdl_gfx", apt: "libsdl-gfx1.2-dev" },
  { name: "zlib", checks: ["zlib"], port: "zlib", apt: "zlib1g-dev" },
];

// Build orchestration.

function runBuild(args) {
  const configuration = getBuildConfiguration(args);
  if (!configuration) return 0;

  const platforms = {
    darwin: { name: "macOS", getSettings: getMacBuildSettings, getGuidance: getMacInstallationGuidance },
    linux: { name: "Linux", getSettings: getLinuxBuildSettings, getGuidance: getLinuxInstallationGuidance },
  };
  const platform = platforms[process.platform];
  if (!platform) throw new Error(`The build helper supports macOS and Linux; received ${process.platform}.`);

  const buildDirectory = path.join(repositoryRoot, "build");
  const cache = readOptionalFile(path.join(buildDirectory, "CMakeCache.txt"));
  const generator = readCacheValue(cache, "CMAKE_GENERATOR");
  if (generator && generator !== "Unix Makefiles") {
    throw new Error("The build helper uses Unix Makefiles. Remove the existing build/CMakeCache.txt and build/CMakeFiles before switching generators.");
  }

  const settings = platform.getSettings(process.env, cache);
  addPackageSearchPaths(settings, cache);

  const tools = {
    cmake: "cmake",
    make: readCacheValue(cache, "CMAKE_MAKE_PROGRAM") || "make",
    pkgConfig: readCacheValue(cache, "PKG_CONFIG_EXECUTABLE") || process.env.PKG_CONFIG || "pkg-config",
  };

  const missing = [...settings.missing, ...checkBuildTools(tools, settings.environment)];
  const hasPkgConfig = !missing.some((item) => item.name === "pkg-config");
  if (hasPkgConfig) missing.push(...checkEngineLibraries(tools.pkgConfig, settings.environment));
  missing.push(...checkCompilers(settings, cache));

  if (missing.length) {
    console.error(`Build prerequisites are missing or unusable:\n${missing.map((item) => `  - ${item.name}`).join("\n")}`);
    if (!hasPkgConfig) console.error("Engine libraries could not be checked. Install pkg-config and rerun pnpm build.");
    console.error(platform.getGuidance(missing));
    return 1;
  }

  const configureArguments = [
    "-S", repositoryRoot, "-B", buildDirectory, "-G", "Unix Makefiles", `-DCMAKE_BUILD_TYPE=${configuration}`,
    `-DCMAKE_MAKE_PROGRAM=${tools.make}`, `-DPKG_CONFIG_EXECUTABLE=${tools.pkgConfig}`, ...settings.configureArguments,
  ];
  if (settings.prefix) configureArguments.push(`-DCMAKE_PREFIX_PATH=${settings.prefix}`);

  console.log(`Building ${configuration} for ${platform.name}.`);
  for (const commandArgs of [configureArguments, ["--build", buildDirectory, "--config", configuration, "--parallel", "2"]]) {
    console.log(`\n> ${tools.cmake} ${commandArgs.join(" ")}`);
    const result = checkCommand(tools.cmake, commandArgs, settings.environment, { stdio: "inherit" });
    if (result.error) throw new Error(`Failed to start ${tools.cmake}: ${result.error.message}`);
    if (result.signal) throw new Error(`Build terminated by ${result.signal}.`);
    if (result.status !== 0) return result.status ?? 1;
  }

  return 0;
}

// Shared build configuration and prerequisite checks.

function getBuildConfiguration(args) {
  const options = args[0] === "--" ? args.slice(1) : args;
  const usage = "Usage: pnpm build [--debug]\nRelease is the default configuration.";

  if (options.some((arg) => !["--debug", "--help"].includes(arg))) throw new Error(usage);

  if (options.includes("--help")) {
    console.log(usage);
    return undefined;
  }

  return options.includes("--debug") ? "Debug" : "Release";
}

function checkBuildTools(tools, environment) {
  const missing = [];

  const cmakeVersion = checkCommand(tools.cmake, ["--version"], environment);
  const version = cmakeVersion.stdout?.match(/cmake version (\d+)\.(\d+)/);
  if (!hasSucceeded(cmakeVersion) || !version || Number(version[1]) < 3 ||
      (Number(version[1]) === 3 && Number(version[2]) < 5)) {
    missing.push({ name: "CMake >= 3.5", brew: "cmake", toolPort: "cmake", apt: "cmake" });
  }

  if (!hasSucceeded(checkCommand(tools.make, ["--version"], environment))) {
    missing.push({ name: "Make", apple: true, apt: "build-essential" });
  }

  if (!hasSucceeded(checkCommand(tools.pkgConfig, ["--version"], environment))) {
    missing.push({ name: "pkg-config", brew: "pkgconf", toolPort: "pkgconfig", apt: "pkg-config" });
  }

  return missing;
}

function checkEngineLibraries(pkgConfig, environment) {
  return libraries.filter((library) => !hasSucceeded(checkCommand(pkgConfig, ["--exists", ...library.checks], environment)));
}

function checkCompilers(settings, cache) {
  const missing = [];
  const temporaryDirectory = mkdtempSync(path.join(tmpdir(), "of-build-check-"));

  try {
    // Compile and link tiny probes, never executing them or building engine sources.
    for (const language of ["C", "CXX"]) {
      const compiler = readCacheValue(cache, `CMAKE_${language}_COMPILER`) ||
        settings.environment[language === "C" ? "CC" : "CXX"] || (language === "C" ? "cc" : "c++");
      const source = language === "C" ? "int main(void) { return 0; }\n" :
        "#include <vector>\n#include <optional>\nstatic_assert(__cplusplus >= 201703L);\nint main() { std::vector<int> v{1}; std::optional<int> n{v[0]}; return *n - 1; }\n";

      const args = [
        ...(language === "CXX" ? ["-std=c++17"] : []), ...settings.compilerArguments,
        "-x", language === "C" ? "c" : "c++", "-", "-o", path.join(temporaryDirectory, language),
      ];

      const result = checkCommand(compiler, args, settings.environment, { input: source });
      if (!hasSucceeded(result)) {
        missing.push({
          name: language === "C" ? "working C compiler" : "working C++17 compiler and standard library",
          apple: true,
          apt: "build-essential",
        });
        console.error(`Compiler check failed (${compiler}): ${result.error?.message || result.stderr?.trim() || "compiler returned an error"}`);
      }
    }
  } finally {
    rmSync(temporaryDirectory, { recursive: true, force: true });
  }

  return missing;
}

function addPackageSearchPaths(settings, cache) {
  const environment = settings.environment;
  const prefixes = [...settings.prefix.split(";"), ...(environment.CMAKE_PREFIX_PATH || "").split(path.delimiter)].filter(Boolean);
  const dependencyDirectory = readCacheValue(cache, "DEPS_DIR");
  const packagePaths = dependencyDirectory && existsSync(dependencyDirectory)
    ? [path.join(dependencyDirectory, "lib", "pkgconfig")]
    : (environment.PKG_CONFIG_PATH || "").split(path.delimiter);

  for (const prefix of prefixes) {
    for (const suffix of ["lib/pkgconfig", "share/pkgconfig"]) {
      const candidate = path.join(prefix, suffix);
      if (existsSync(candidate)) packagePaths.push(candidate);
    }
  }

  const searchPath = getUniqueValues(packagePaths).join(path.delimiter);
  if (searchPath) environment.PKG_CONFIG_PATH = searchPath;
}

// Shared command and data helpers.

function checkCommand(command, args, environment, options = {}) {
  return spawnSync(command, args, { cwd: repositoryRoot, env: environment, encoding: "utf8", ...options });
}

function readOptionalFile(filename) {
  try {
    return readFileSync(filename, "utf8");
  } catch (error) {
    if (error.code !== "ENOENT") throw error;
    return "";
  }
}

function readCacheValue(cache, name) {
  return cache.match(new RegExp(`^${name}:[^=]+=([^\\r\\n]*)`, "m"))?.[1];
}

function hasSucceeded(result) {
  return !result.error && result.status === 0;
}

function getUniqueValues(values) {
  return [...new Set(values.filter(Boolean))];
}

// macOS: discover Homebrew/MacPorts tools, select the SDK, and explain installation.

function getMacBuildSettings(environment, cache) {
  const childEnvironment = { ...environment };
  childEnvironment.PATH = getUniqueValues([
    ...(environment.PATH || "").split(path.delimiter),
    ...["/opt/local/bin", "/opt/homebrew/bin", "/usr/local/bin"].filter(existsSync),
  ]).join(path.delimiter);

  const settings = {
    environment: childEnvironment,
    prefix: readCacheValue(cache, "CMAKE_PREFIX_PATH") ||
      (!environment.CMAKE_PREFIX_PATH && existsSync("/opt/local") ? "/opt/local" : ""),
    compilerArguments: [],
    configureArguments: [],
    missing: [],
  };

  const sdk = getMacSdk(childEnvironment, cache);
  if (sdk) {
    childEnvironment.SDKROOT = sdk;
    settings.configureArguments.push(`-DCMAKE_OSX_SYSROOT=${sdk}`);
    settings.compilerArguments.push("-isysroot", sdk);
  } else {
    settings.missing.push({ name: "macOS SDK with C++ headers", apple: true });
  }

  return settings;
}

function getMacInstallationGuidance(missing) {
  const lines = [];
  const tools = missing.filter((item) => item.brew || item.toolPort);

  if (tools.length) {
    lines.push("Install missing build tools using either Homebrew OR MacPorts:",
      `  brew install ${getUniqueValues(tools.map((item) => item.brew)).join(" ")}`,
      `  sudo port install ${getUniqueValues(tools.map((item) => item.toolPort)).join(" ")}`,
      "Homebrew: https://brew.sh");
  }

  const ports = getUniqueValues(missing.map((item) => item.port));
  if (ports.length) {
    lines.push("Install missing engine libraries using MacPorts:", `  sudo port install ${ports.join(" ")}`);
  }
  if (tools.length || ports.length) lines.push("MacPorts: https://www.macports.org/install.php");

  if (missing.some((item) => item.apple)) {
    lines.push("Install/select Apple Command Line Tools and a usable macOS SDK:", "  xcode-select --install");
  }

  return lines.join("\n");
}

function getMacSdk(environment, cache) {
  const selectedSdk = readCacheValue(cache, "CMAKE_OSX_SYSROOT") || environment.SDKROOT;
  let sdk = selectedSdk;

  if (!sdk || !existsSync(sdk)) {
    const result = checkCommand("xcrun", [...(selectedSdk ? ["--sdk", selectedSdk] : []), "--show-sdk-path"], environment);
    sdk = hasSucceeded(result) ? result.stdout.trim() : undefined;
  }

  return sdk && existsSync(path.join(sdk, "usr", "include", "c++", "v1", "vector")) ? sdk : undefined;
}

// Linux: keep system tool discovery and choose distro-appropriate installation guidance.

function getLinuxBuildSettings(environment, cache) {
  return {
    environment: { ...environment },
    prefix: readCacheValue(cache, "CMAKE_PREFIX_PATH") || "",
    compilerArguments: [],
    configureArguments: [],
    missing: [],
  };
}

function getLinuxInstallationGuidance(missing) {
  if (isDebianFamily(readOptionalFile("/etc/os-release"))) {
    return `Install missing prerequisites:\n  sudo apt install ${getUniqueValues(missing.map((item) => item.apt)).join(" ")}`;
  }

  return [
    "Install these tools/development libraries using your distribution's package manager:",
    ...getUniqueValues(missing.map((item) => item.name)).map((name) => `  - ${name}`),
    "Use SDL 1.2 development packages (or sdl12-compat), including headers and pkg-config files; SDL2 alone is insufficient.",
  ].join("\n");
}

function isDebianFamily(osRelease) {
  const values = {};

  for (const line of osRelease.split(/\r?\n/)) {
    const match = line.match(/^(ID|ID_LIKE)=(.*)$/);
    if (match) values[match[1]] = match[2].replace(/^(["'])(.*)\1$/, "$2");
  }

  return `${values.ID || ""} ${values.ID_LIKE || ""}`.split(/\s+/).some((id) => ["ubuntu", "debian"].includes(id));
}

// Script execution.

try {
  process.exitCode = runBuild(process.argv.slice(2));
} catch (error) {
  console.error(error.message);
  process.exitCode = 1;
}
