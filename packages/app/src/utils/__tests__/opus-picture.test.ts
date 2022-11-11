import { describe, expect, it } from "vitest";
import { decodeJpeg } from "../opus-picture";
import fs from "fs";

describe("opus-picture", () => {
  describe("decodeJpeg", () => {
    it("success", async () => {
      const data = await fs.promises.readFile("../../misc/test.jpg");
      const result = decodeJpeg(data);
      expect(result).toMatchInlineSnapshot(`
        {
          "colors": 0,
          "depth": 24,
          "height": 360,
          "mimeType": "image/jpeg",
          "width": 480,
        }
      `);
    });
  });
});
