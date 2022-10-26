const path = require("path");
const fs = require("fs");
const assert = require("assert/strict");
const { importEmscriptenModule, Cli } = require("./emscripten-utils.js");

function readFileToVector(vector, filename) {
  const buffer = fs.readFileSync(filename);
  vector.resize(buffer.length, 0);
  vector.view().set(new Uint8Array(buffer));
  return vector;
}

async function main() {
  const cli = new Cli(process.argv.slice(2));
  const modulePath = cli.argument("--module");
  const inFile = cli.argument("--in");
  const inPictureFile = cli.argument("--in-picture");
  const inMetadata = cli.argument("--in-metadata");
  const outFile = cli.argument("--out");
  const outFormat = outFile.split(".").at(-1);

  // initialize wasm
  const lib = await importEmscriptenModule(path.resolve(modulePath));

  // load file data into wasm heap via embind vector
  const inData = readFileToVector(new lib.Vector(), inFile);

  // metadata
  const metadata = new lib.StringMap();
  if (inMetadata) {
    for (const [k, v] of Object.entries(JSON.parse(inMetadata))) {
      assert.ok(typeof v === "string");
      metadata.set(k, v);
    }
  }
  if (inPictureFile) {
    const pictureData = readFileToVector(new lib.Vector(), inPictureFile);
    const encoded = lib.encodePictureMetadata(pictureData);
    metadata.set("METADATA_BLOCK_PICTURE", encoded);
  }

  // run
  const outData = lib.convert(inData, outFormat, metadata);

  // write file
  fs.writeFileSync(outFile, outData.view());
}

if (require.main === module) {
  main();
}
