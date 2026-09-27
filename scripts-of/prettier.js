import { spawnSync } from "node:child_process";
import { readdirSync, realpathSync, statSync } from "node:fs";
import { dirname, extname, join, relative, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";

const projectRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const sourceRoot = realpathSync(join(projectRoot, "src", "OppositeForce"));
const cppExtensions = new Set([".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".ipp", ".tpp"]);

function isInSourceRoot(filePath) {
  const pathFromSourceRoot = relative(sourceRoot, filePath);
  return pathFromSourceRoot !== "" && pathFromSourceRoot !== ".." && !pathFromSourceRoot.startsWith(`..${sep}`);
}

function collectCppFiles(directory) {
  const files = [];
  for (const entry of readdirSync(directory, { withFileTypes: true })) {
    const entryPath = join(directory, entry.name);
    if (entry.isDirectory()) {
      files.push(...collectCppFiles(entryPath));
    } else if (entry.isFile() && cppExtensions.has(extname(entry.name).toLowerCase())) {
      files.push(entryPath);
    }
  }
  return files;
}

function resolveRequestedFiles(argumentsAfterScript) {
  const fileArguments = argumentsAfterScript[0] === "--" ? argumentsAfterScript.slice(1) : argumentsAfterScript;
  if (fileArguments.length === 0) {
    return collectCppFiles(sourceRoot);
  }

  return fileArguments.map((fileArgument) => {
    const filePath = realpathSync(resolve(fileArgument));
    if (!isInSourceRoot(filePath) || !statSync(filePath).isFile() || !cppExtensions.has(extname(filePath).toLowerCase())) {
      throw new Error(`Only C++ files in src/OppositeForce can be formatted: ${fileArgument}`);
    }
    return filePath;
  });
}

try {
  const files = resolveRequestedFiles(process.argv.slice(2));
  if (files.length === 0) {
    console.log("No OppositeForce C++ files found.");
    process.exit(0);
  }

  const result = spawnSync("clang-format", ["--style=file", "-i", ...files], { stdio: "inherit" });
  if (result.error) {
    throw result.error;
  }
  process.exit(result.status ?? 1);
} catch (error) {
  console.error(error.message);
  process.exit(1);
}
