interface EmscriptenModuleArg {
  locateFile: () => string;
}

interface EmscriptenModule {
  wasmToOpus: () => void;
}

type EmscriptenExport = (arg: EmscriptenModuleArg) => Promise<EmscriptenModule>;

export = EmscriptenExport;
