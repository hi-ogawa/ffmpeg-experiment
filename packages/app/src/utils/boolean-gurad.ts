// usage:
//   ["x", "y", undefined, null].filter(booleanGuard): string[]

type BooleanGuard = <T>(x: T) => x is NonNullable<T>;

export const booleanGuard = Boolean as any as BooleanGuard;
