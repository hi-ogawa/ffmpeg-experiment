export function tinyassert(value: any): asserts value {
  if (!value) {
    throw new Error("tinyassert");
  }
}
