import { spawnSync } from "node:child_process";
import path from "node:path";
import { fileURLToPath } from "node:url";

const scriptDirectory = path.dirname(fileURLToPath(import.meta.url));
const repositoryRoot = path.resolve(scriptDirectory, "..");

if (process.platform !== "darwin") {
  console.error("The pnpm build script currently supports macOS only.");
  process.exit(1);
}

function run(command, args) {
  console.log(`\n> ${command} ${args.join(" ")}`);

  const result = spawnSync(command, args, {
    cwd: repositoryRoot,
    stdio: "inherit",
  });

  if (result.error) {
    console.error(`Failed to start ${command}: ${result.error.message}`);
    process.exit(1);
  }

  if (result.status !== 0) {
    process.exit(result.status ?? 1);
  }
}

run("cmake", [
  "-G",
  "Unix Makefiles",
  "-DCMAKE_BUILD_TYPE=Release",
  "-DCMAKE_PREFIX_PATH=/opt/local",
  "-B",
  "build",
  ".",
]);

run("make", ["-C", "build", "-j2"]);
