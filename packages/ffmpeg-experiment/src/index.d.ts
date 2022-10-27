//
// exports
//

class Vector {
  resize(length: number, fillValue: number): void;
  view(): Uint8Array;
}

class StringMap {
  set(k: string, v: string): void;
}

const convert: (
  inData: Vector,
  outFormat: string,
  metadata: StringMap
) => Vector;

const encodePictureMetadata: (inData: Vector) => string;

const moduleExports = { Vector, StringMap, convert, encodePictureMetadata };

export type ModuleExports = typeof moduleExports;

//
// MODULARIZE entrypoint
//

function init(arg: initArg): Promise<ModuleExports>;
export = init;
