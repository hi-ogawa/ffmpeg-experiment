const fs = require("fs");

class Cli {
  constructor(argv) {
    this.argv = argv;
  }

  argument(flag) {
    const index = this.argv.indexOf(flag);
    if (index !== -1) {
      return this.argv[index + 1];
    }
  }
}

function readFileToVector(vector, filename) {
  const buffer = fs.readFileSync(filename);
  vector.resize(buffer.length, 0);
  vector.view().set(new Uint8Array(buffer));
  return vector;
}

module.exports = { Cli, readFileToVector };
