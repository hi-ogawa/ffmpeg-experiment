//
// embed image data (e.g. cover art) to ogg/opus metadata
//
// cf.
// - https://wiki.xiph.org/VorbisComment#Cover_art
// - https://xiph.org/flac/format.html#metadata_block_picture
// - https://gitlab.xiph.org/xiph/libopusenc/-/blob/f51c3aa431c2f0f8fccd8926628b5f330292489f/src/picture.c
// - https://github.com/FFmpeg/FFmpeg/blob/81bc4ef14292f77b7dcea01b00e6f2ec1aea4b32/libavformat/flacenc.c#L81

import { tinyassert } from "./tinyassert";

export const METADATA_BLOCK_PICTURE = "METADATA_BLOCK_PICTURE";

// image metadata extraction and base64 encoding has to be done by the caller
export function encodeOpusPicture(
  data: Uint8Array,
  info: ImageInfo,
  pictureType: number,
  description: string
): Uint8Array {
  // prettier-ignore
  const numBytes =
    // picture type
    4 +
    // mime type
    4 + info.mimeType.length +
    // description
    4 + description.length +
    // image info
    4 + 4 + 4 + 4 +
    // data
    4 + data.length;
  tinyassert(numBytes < 2 ** 24);

  const writer = new BytesWriter(new Uint8Array(numBytes));

  writer.write(bytesFromU32BE(pictureType));

  writer.write(bytesFromU32BE(info.mimeType.length));
  writer.write(bytesFromString(info.mimeType));

  writer.write(bytesFromU32BE(description.length));
  writer.write(bytesFromString(description));

  writer.write(bytesFromU32BE(info.width));
  writer.write(bytesFromU32BE(info.height));
  writer.write(bytesFromU32BE(info.depth));
  writer.write(bytesFromU32BE(info.colors));

  writer.write(bytesFromU32BE(data.length));
  writer.write(data);

  return writer.data;
}

export interface ImageInfo {
  mimeType: string;
  width: number;
  height: number;
  depth: number;
  colors: number;
}

//
// bytes io utility
//

class BytesWriter {
  private offset: number = 0;

  constructor(public data: Uint8Array) {}

  write(newData: Uint8Array) {
    this.data.set(newData, this.offset);
    this.offset += newData.length;
  }
}

class BytesReader {
  private offset: number = 0;

  constructor(public data: Uint8Array) {}

  read(size: number): Uint8Array {
    const nextOffset = this.offset + size;
    tinyassert(nextOffset <= this.data.length);
    const result = this.data.slice(this.offset, nextOffset);
    this.offset = nextOffset;
    return result;
  }
}

function bytesFromString(value: string): Uint8Array {
  return new TextEncoder().encode(value);
}

function bytesFromU32BE(value: number): Uint8Array {
  return new Uint8Array([value >> 24, value >> 16, value >> 8, value]);
}

function bytesToU16BE(value: Uint8Array): number {
  tinyassert(value.length === 2);
  return (value[0]! << 8) | value[1]!;
}

//
// jpeg metadata parser
// - https://github.com/nothings/stb/blob/8b5f1f37b5b75829fc72d38e7b5d4bcbf8a26d55/stb_image.h#L3330
// - https://gitlab.xiph.org/xiph/libopusenc/-/blob/f51c3aa431c2f0f8fccd8926628b5f330292489f/src/picture.c#L189

export function decodeJpeg(data: Uint8Array): ImageInfo {
  const reader = new BytesReader(data);

  //
  // cf. stbi__get_marker
  //
  const NONE = 0xff;

  function next(): number {
    let m = reader.read(1)[0]!;
    if (m !== NONE) {
      return NONE;
    }
    while (m === NONE) {
      m = reader.read(1)[0]!;
    }
    return m;
  }

  //
  // cf. stbi__decode_jpeg_header
  //
  let m = next();

  // SOI
  tinyassert(m === 0xd8);

  while (true) {
    m = next();
    // SOF
    if (0xc0 <= m && m <= 0xc2) {
      break;
    }
    // restart marker
    if (0xd0 <= m && m <= 0xd7) {
      continue;
    }
    // check valid marker and skip payload
    tinyassert(
      (0xe0 <= m && m <= 0xef) || m === 0xfe || [0xc4, 0xdb, 0xdd].includes(m)
    );
    const L = bytesToU16BE(reader.read(2));
    tinyassert(L >= 2);
    reader.read(L - 2);
  }

  //
  // cf. stbi__process_frame_header
  //
  const Lf = bytesToU16BE(reader.read(2));
  tinyassert(Lf >= 11);
  const p = reader.read(1)[0]!;
  tinyassert(p === 8);
  const img_y = bytesToU16BE(reader.read(2));
  const img_x = bytesToU16BE(reader.read(2));
  const c = reader.read(1)[0]!;

  return {
    mimeType: "image/jpeg",
    width: img_x,
    height: img_y,
    depth: c * p,
    colors: 0,
  };
}
