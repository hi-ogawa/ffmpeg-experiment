import process from "process";
import path from "path";
import fs from "fs";
import { Cli } from "./emscripten-utils";
import assert from "assert";

interface FileInfo {
  node: string;
  emscripten: string;
}

function copyNodeToEmscripten(FS: any, file: FileInfo) {
  const data = fs.readFileSync(file.node);
  FS.writeFile(file.emscripten, data);
}

function copyEmscriptenToNode(FS: any, file: FileInfo) {
  const data = FS.readFile(file.emscripten);
  fs.writeFileSync(file.node, data);
}

async function main() {
  const cli = new Cli(process.argv.slice(2));
  const modulePath = cli.argument("--module");
  const inFile = cli.argument("--in");
  const outFile = cli.argument("--out");
  const artist = cli.argument("--artist");
  const title = cli.argument("--title");
  assert.ok(modulePath);
  assert.ok(inFile);
  assert.ok(outFile);

  //
  // initialize emscripten module
  //

  const init = require(path.resolve(modulePath));

  let exitCodeResolve: (value: number) => void;
  const exitCodePromise = new Promise<number>((resolve) => {
    exitCodeResolve = resolve;
  });

  const Module = await init({
    noInitialRun: true,
    quit: (exitCode: number) => {
      exitCodeResolve(exitCode);
    },
  });

  //
  // run ffmpeg main
  //

  // prettier-ignore
  const args = [
    "-i", "/in.webm",
    "-c", "copy",
    artist && ["-metadata", `artist=${artist}`],
    title  && ["-metadata", `title=${title}`],
    "/out.opus",
  ].flat().filter(Boolean);

  const inFiles: FileInfo[] = [
    {
      node: inFile,
      emscripten: "/in.webm",
    },
  ];

  const outFiles: FileInfo[] = [
    {
      node: outFile,
      emscripten: "/out.opus",
    },
  ];

  // copy files to emscripten
  inFiles.forEach((f) => copyNodeToEmscripten(Module.FS, f));

  // invoke main and wait
  Module.callMain(args);
  const exitCode = await exitCodePromise;
  console.log({ exitCode });

  // copy files from emscripten
  outFiles.forEach((f) => copyEmscriptenToNode(Module.FS, f));

  // cleanup hanging threads (otherwise nodejs doesn't exit)
  Module.PThread.terminateAllThreads();
}

if (require.main === module) {
  main();
}
