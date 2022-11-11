import process from "process";
import path from "path";
import { Cli } from "./emscripten-utils";

async function main() {
  const cli = new Cli(process.argv.slice(2));
  const modulePath = cli.argument("--module");

  // initialize emscripten module
  const init = require(path.resolve(modulePath));
  let exitCodeResolve: (value: number) => void;
  const exitCodePromise = new Promise<number>((resolve) => {
    exitCodeResolve = resolve;
  });
  function quit(exitCode: number) {
    exitCodeResolve(exitCode);
  }
  const Module = await init({ noInitialRun: true, quit });

  // run ffmpeg main
  Module.callMain(["--help"]);
  const exitCode = await exitCodePromise;
  console.log({ exitCode });
  Module.PThread.terminateAllThreads();
}

if (require.main === module) {
  main();
}
