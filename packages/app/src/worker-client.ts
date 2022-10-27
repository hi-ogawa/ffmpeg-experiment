import { wrap } from "comlink";
import _ from "lodash";
import WorkerConstructor from "./worker-impl?worker";
import type { WorkerImpl } from "./worker-impl";

export const getWorker = _.memoize(async () => {
  const worker = wrap<WorkerImpl>(new WorkerConstructor());
  await worker.initialize();
  return worker;
});
