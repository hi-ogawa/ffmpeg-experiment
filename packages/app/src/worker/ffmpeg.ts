// NOTE:
// it seems vite cannot bundle emscripten proxy-to-pthread build by import "?worker" (at least on dev).
// so, we transpile `src/worker/ffmpeg.ts` separately and uses it via "?url" import.

import { expose } from "comlink";
import { run, RunOptions, RunResult } from "./run";

// export type for comlink
export type { FFmpegWorker };

class FFmpegWorker {
  // flatten arguments for comlink callback proxy to work
  async run(
    options: Omit<RunOptions, "initModule" | "onStdout" | "onStderr">,
    onStdout?: RunOptions["onStdout"],
    onStderr?: RunOptions["onStderr"]
  ): Promise<RunResult> {
    importScripts(options.moduleUrl);
    const initModule = (self as any).ffmpeg;
    return run({ ...options, initModule, onStdout, onStderr });
  }
}

function main() {
  const worker = new FFmpegWorker();
  expose(worker);
}

main();
