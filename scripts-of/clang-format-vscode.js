#!/usr/bin/env node
import { spawnSync } from "node:child_process";
import { readFileSync, realpathSync } from "node:fs";
import { basename, dirname, extname, isAbsolute, join, relative, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";

// Only VS Code uses this wrapper. Direct clang-format calls retain the upstream style.
// The C/C++ extension sends the editor buffer on stdin with --assume-filename.
const projectRoot = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const sourceRoot = realpathSync(join(projectRoot, "src", "OppositeForce"));
const cppExtensions = new Set([".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".ipp", ".tpp"]);
const argumentsAfterScript = process.argv.slice(2);

function getArgumentValue(option) {
  for (let index = 0; index < argumentsAfterScript.length; index++) {
    const argument = argumentsAfterScript[index];
    if (argument.startsWith(`--${option}=`) || argument.startsWith(`-${option}=`)) {
      return argument.slice(argument.indexOf("=") + 1);
    }
    if (argument === `--${option}` || argument === `-${option}`) {
      return argumentsAfterScript[index + 1];
    }
  }
  return undefined;
}

function canFormatFile(filename) {
  if (!filename || !cppExtensions.has(extname(filename).toLowerCase())) {
    return false;
  }
  try {
    const filePath = resolve(filename);
    let canonicalPath;
    try {
      canonicalPath = realpathSync(filePath);
    } catch (error) {
      if (error.code !== "ENOENT") {
        throw error;
      }
      // Allow new editor files, while resolving directory symlinks before checking scope.
      canonicalPath = join(realpathSync(dirname(filePath)), basename(filePath));
    }
    const pathFromSourceRoot = relative(sourceRoot, canonicalPath);
    return (
      pathFromSourceRoot !== "" &&
      pathFromSourceRoot !== ".." &&
      !pathFromSourceRoot.startsWith(`..${sep}`) &&
      !isAbsolute(pathFromSourceRoot)
    );
  } catch {
    return false;
  }
}

if (argumentsAfterScript.includes("--version") || canFormatFile(getArgumentValue("assume-filename"))) {
  const result = spawnSync("clang-format", argumentsAfterScript, { stdio: "inherit" });
  if (result.error) {
    console.error(result.error.message);
  }
  process.exit(result.status ?? 1);
}

const input = readFileSync(0);
const outputXml = argumentsAfterScript.some((argument) => /^--?output-replacements-xml(?:=true)?$/.test(argument));
const cursorArgument = getArgumentValue("cursor");
const cursor = cursorArgument && /^\d+$/.test(cursorArgument) ? Number(cursorArgument) : undefined;
if (outputXml) {
  process.stdout.write('<?xml version="1.0"?>\n<replacements xml:space="preserve" incomplete_format="false">\n');
  if (cursor !== undefined) {
    process.stdout.write(`<cursor>${cursor}</cursor>\n`);
  }
  process.stdout.write("</replacements>\n");
} else {
  if (cursor !== undefined) {
    process.stdout.write(`{ "Cursor": ${cursor}, "IncompleteFormat": false }\n`);
  }
  process.stdout.write(input);
}
