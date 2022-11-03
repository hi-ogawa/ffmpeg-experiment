import { wrap, proxy } from "comlink";
import _ from "lodash";

import WORKER_URL from "./worker/build/ffmpeg.js?url";
import type { FFmpegWorker } from "./worker/ffmpeg";
import type { RunOptions } from "./worker/run";

import FFMPEG_MODULE_URL from "@hiogawa/ffmpeg-experiment/build/ffmpeg_g.js?url";
import FFMPEG_WORKER_URL from "@hiogawa/ffmpeg-experiment/build/ffmpeg_g.worker.js?url";
import FFMPEG_WASM_URL from "@hiogawa/ffmpeg-experiment/build/ffmpeg_g.wasm?url";

type ClientRunOptions = Pick<
  RunOptions,
  "arguments" | "inFiles" | "outFiles" | "onStdout" | "onStderr"
>;

export async function runFFmpegWorker(options: ClientRunOptions) {
  const worker = new Worker(WORKER_URL);

  // proxy callbacks
  const onStdout = options.onStdout && proxy(options.onStdout);
  const onStderr = options.onStderr && proxy(options.onStderr);
  options = _.omit(options, ["onStdout", "onStderr"]);

  try {
    const workerImpl = wrap<FFmpegWorker>(worker);
    const result = await workerImpl.run(
      {
        ...options,
        moduleUrl: FFMPEG_MODULE_URL,
        wasmUrl: FFMPEG_WASM_URL,
        workerUrl: FFMPEG_WORKER_URL,
      },
      onStdout,
      onStderr
    );
    return result;
  } finally {
    worker.terminate();
  }
}

// prefetch heavy assets
export const WORKER_ASSET_URLS = [
  WORKER_URL,
  FFMPEG_MODULE_URL,
  FFMPEG_WORKER_URL,
  FFMPEG_WASM_URL,
];
