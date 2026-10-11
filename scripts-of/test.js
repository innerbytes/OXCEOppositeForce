import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const repositoryRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const testsDirectory = path.join(repositoryRoot, "src", "OppositeForce", "Tests");
const buildDirectory = path.join(testsDirectory, "build");

if (process.platform !== "darwin") {
  console.error("The pnpm test script currently supports macOS only.");
  process.exit(1);
}

// Prefer an explicitly selected SDK. Otherwise use the active SDK, with the
// Command Line Tools SDK as a fallback for incomplete Xcode installations.
const sdkResult = spawnSync("xcrun", ["--show-sdk-path"], { encoding: "utf8" });
const sdkCandidates = [
  process.env.SDKROOT,
  sdkResult.status === 0 ? sdkResult.stdout.trim() : undefined,
  "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk",
];
const sdk = sdkCandidates.find((candidate) => candidate && existsSync(path.join(candidate, "usr", "include", "c++", "v1", "vector")));
if (!sdk) {
  console.error("A macOS SDK with C++ headers is required. Select it using SDKROOT.");
  process.exit(1);
}

function run(command, args, phase) {
  const started = performance.now();
  const result = spawnSync(command, args, {
    cwd: repositoryRoot,
    env: { ...process.env, SDKROOT: sdk },
    stdio: "inherit",
  });
  if (result.error) {
    console.error(`Failed to start ${command}: ${result.error.message}`);
    process.exit(1);
  }
  if (result.status !== 0) process.exit(result.status ?? 1);
  console.log(`${phase}: ${((performance.now() - started) / 1000).toFixed(2)}s`);
}

run("cmake", [
  "-S", repositoryRoot,
  "-B", buildDirectory,
  "-G", "Unix Makefiles",
  "-DCMAKE_BUILD_TYPE=Release",
  "-DCMAKE_PREFIX_PATH=/opt/local",
  `-DCMAKE_OSX_SYSROOT=${sdk}`,
  `-DCMAKE_PROJECT_OpenXcom_INCLUDE=${path.join(testsDirectory, "RegisterTests.cmake")}`,
  "-DBUILD_PACKAGE=OFF",
  "-DCREATE_BUNDLE=OFF",
  "-DEMBED_ASSETS=OFF",
  "-DDUMP_CORE=OFF",
], "Configure");
run("cmake", ["--build", buildDirectory, "--target", "opposite_force_tests", "--parallel", "2"], "Build");

const argumentsAfterScript = process.argv.slice(2);
const testArguments = argumentsAfterScript[0] === "--" ? argumentsAfterScript.slice(1) : argumentsAfterScript;
run(path.join(buildDirectory, "tests", "opposite_force_tests"), testArguments, "Tests");
