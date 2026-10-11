import { spawnSync } from "node:child_process";
import { existsSync, readFileSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

function readCacheValue(cache, name) {
  return cache.match(new RegExp(`^${name}:[^=]+=([^\\r\\n]*)`, "m"))?.[1];
}

function readConfigureArguments(value) {
  let args;
  try {
    args = value ? JSON.parse(value) : [];
  } catch {
    throw new Error("OF_TEST_CMAKE_ARGS must contain valid JSON.");
  }
  if (!Array.isArray(args) || args.some((arg) => typeof arg !== "string")) {
    throw new Error("OF_TEST_CMAKE_ARGS must be a JSON array of CMake argument strings.");
  }
  return args;
}

function runTests() {
  const platform = process.platform;
  const environment = process.env;
  const repositoryRoot = fileURLToPath(new URL("../", import.meta.url));
  const argumentsAfterScript = process.argv.slice(2);
  if (!["darwin", "linux", "win32"].includes(platform)) {
    throw new Error(`The test helper supports macOS, Linux, and Windows; received ${platform}.`);
  }

  const paths = platform === "win32" ? path.win32 : path.posix;
  const toCmakePath = (value) => platform === "win32" ? value.replaceAll("\\", "/") : value;
  const testsDirectory = paths.join(repositoryRoot, "src", "OppositeForce", "Tests");
  const buildDirectory = paths.join(testsDirectory, "build");
  const cacheFile = paths.join(buildDirectory, "CMakeCache.txt");
  const cache = existsSync(cacheFile) ? readFileSync(cacheFile, "utf8") : "";
  const generator = environment.CMAKE_GENERATOR || readCacheValue(cache, "CMAKE_GENERATOR") ||
    (platform === "win32" ? "Ninja" : "Unix Makefiles");
  const extraArguments = readConfigureArguments(environment.OF_TEST_CMAKE_ARGS);
  const configureArguments = ["-S", toCmakePath(repositoryRoot), "-B", toCmakePath(buildDirectory), "-G", generator];
  const childEnvironment = { ...environment };

  if (platform === "darwin") {
    const sdkResult = spawnSync("xcrun", ["--show-sdk-path"], { encoding: "utf8", env: childEnvironment });
    const sdkCandidates = [
      environment.SDKROOT,
      sdkResult.status === 0 ? sdkResult.stdout.trim() : undefined,
      "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk",
    ];
    const sdk = sdkCandidates.find((candidate) => candidate && existsSync(paths.join(candidate, "usr", "include", "c++", "v1", "vector")));
    if (!sdk) throw new Error("A macOS SDK with C++ headers is required. Select it using SDKROOT.");
    childEnvironment.SDKROOT = sdk;
    configureArguments.push(`-DCMAKE_OSX_SYSROOT=${sdk}`);
    if (!environment.CMAKE_PREFIX_PATH && !readCacheValue(cache, "CMAKE_PREFIX_PATH")) {
      configureArguments.push("-DCMAKE_PREFIX_PATH=/opt/local");
    }
  }

  const hasCompilerSelection = environment.CC || environment.CXX || environment.CMAKE_TOOLCHAIN_FILE ||
    extraArguments.some((arg) => /^-DCMAKE_(C_COMPILER|CXX_COMPILER|TOOLCHAIN_FILE)(:|=)/.test(arg));
  if (platform === "win32" && !cache && !hasCompilerSelection) {
    if (generator.startsWith("Visual Studio")) {
      if (!environment.CMAKE_GENERATOR_TOOLSET) configureArguments.push("-T", "ClangCL");
    } else if (["Ninja", "Ninja Multi-Config", "NMake Makefiles"].includes(generator)) {
      configureArguments.push("-DCMAKE_C_COMPILER=clang-cl", "-DCMAKE_CXX_COMPILER=clang-cl");
    }
  }
  // CMake 3.19 does not initialize this cache variable from the environment.
  if (environment.CMAKE_TOOLCHAIN_FILE) {
    configureArguments.push(`-DCMAKE_TOOLCHAIN_FILE=${toCmakePath(environment.CMAKE_TOOLCHAIN_FILE)}`);
  }
  configureArguments.push(...extraArguments,
    "-DCMAKE_BUILD_TYPE=Release",
    `-DCMAKE_PROJECT_OpenXcom_INCLUDE=${toCmakePath(paths.join(testsDirectory, "RegisterTests.cmake"))}`,
    "-DBUILD_PACKAGE=OFF", "-DCREATE_BUNDLE=OFF", "-DEMBED_ASSETS=OFF", "-DDUMP_CORE=OFF");

  function run(command, args, phase, env = childEnvironment) {
    const started = performance.now();
    const result = spawnSync(command, args, { cwd: repositoryRoot, env, stdio: "inherit" });
    if (result.error) throw new Error(`Failed to start ${command}: ${result.error.message}`);
    if (result.signal) throw new Error(`${phase} terminated by ${result.signal}.`);
    console.log(`${phase}: ${((performance.now() - started) / 1000).toFixed(2)}s`);
    return result.status ?? 1;
  }

  const configureStatus = run("cmake", configureArguments, "Configure");
  if (configureStatus !== 0) return configureStatus;
  const buildStatus = run("cmake", ["--build", buildDirectory, "--target", "opposite_force_tests",
    "--config", "Release", "--parallel", "2"], "Build");
  if (buildStatus !== 0) return buildStatus;

  // CMake supplies the actual path, including .exe and any Release subdirectory.
  const manifest = paths.join(buildDirectory, "test-runner-Release.txt");
  const [executable, ...runtimeDirectories] = readFileSync(manifest, "utf8").split(/\r?\n/).filter(Boolean);
  if (!executable || !paths.isAbsolute(executable) || !existsSync(executable)) {
    throw new Error(`The test executable recorded in ${manifest} does not exist.`);
  }
  const runtimeEnvironment = { ...childEnvironment };
  if (platform === "win32") {
    // Preserve Windows' existing key spelling; duplicate Path/PATH keys are ambiguous to Node.
    const pathKeys = Object.keys(runtimeEnvironment).filter((key) => key.toLowerCase() === "path").sort();
    const pathKey = pathKeys[0] || "PATH";
    const originalPath = runtimeEnvironment[pathKey];
    for (const key of pathKeys) delete runtimeEnvironment[key];
    runtimeEnvironment[pathKey] = [...runtimeDirectories, originalPath].filter(Boolean).join(paths.delimiter);
  } else if (platform === "linux" && runtimeDirectories.length) {
    runtimeEnvironment.LD_LIBRARY_PATH = [...runtimeDirectories, runtimeEnvironment.LD_LIBRARY_PATH].filter(Boolean).join(paths.delimiter);
  }
  const testArguments = argumentsAfterScript[0] === "--" ? argumentsAfterScript.slice(1) : argumentsAfterScript;
  return run(executable, testArguments, "Tests", runtimeEnvironment);
}

try {
  process.exitCode = runTests();
} catch (error) {
  console.error(error.message);
  process.exitCode = 1;
}
