function importEmscriptenModule(modulePath) {
  return new Promise((resolve) => {
    const Module = require(modulePath);
    Module.onRuntimeInitialized = () => {
      resolve(Module);
    };
  });
}

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

module.exports = { importEmscriptenModule, Cli };
