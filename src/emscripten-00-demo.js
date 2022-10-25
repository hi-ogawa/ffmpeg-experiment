const path = require("path");
const fs = require("fs");
const assert = require("assert/strict");

function importEmscriptenModule(modulePath) {
  return new Promise((resolve) => {
    const Module = require(modulePath);
    Module.onRuntimeInitialized = () => {
      resolve(Module);
    };
  });
}

async function main() {
  const [modulePath, inFile] = process.argv.slice(2);
  assert.ok(modulePath);
  assert.ok(inFile);

  // initialize wasm
  const lib = await importEmscriptenModule(path.resolve(modulePath));

  // load file data into wasm heap via embind vector
  const v = new lib.Vector();
  const f = fs.readFileSync(inFile);
  v.resize(f.length, 0);
  v.view().set(new Uint8Array(f));

  // run
  const res = lib.runTest(v);
  console.log(res);
}

if (require.main === module) {
  main();
}
