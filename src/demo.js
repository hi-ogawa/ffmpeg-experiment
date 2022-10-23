const path = require("path");
const fs = require("fs");

function importEmscriptenModule(jsPath) {
  return new Promise((resolve) => {
    const Module = require(jsPath);
    Module.onRuntimeInitialized = () => {
      resolve(Module);
    };
  });
}

async function main() {
  // initialize wasm
  const lib = await importEmscriptenModule(
    "../build/emscripten/Debug/emscripten-00.js"
  );

  // load file data into wasm heap via embind vector
  const v = new lib.Vector();
  const f = fs.readFileSync("test.webm");
  v.resize(f.length, 0);
  v.view().set(new Uint8Array(f));

  // run
  const res = lib.getFormatMetadata(v);

  console.log(res);
}

// node -r ./src/demo.js
// lib = await importEmscriptenModule(path.resolve("./build/emscripten/Debug/emscripten-00.js"))
// Object.assign(globalThis, { importEmscriptenModule });

if (require.main === module) {
  main();
} else {
  Object.assign(globalThis, { importEmscriptenModule });
}
