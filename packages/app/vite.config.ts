import react from "@vitejs/plugin-react";
import { defineConfig } from "vite";
import windicss from "vite-plugin-windicss";

export default defineConfig({
  base: "./",
  plugins: [windicss(), react()],
  optimizeDeps: {
    include: ["@hiogawa/ffmpeg-experiment"],
  },
  build: {
    commonjsOptions: {
      // TODO: how to specify scoped package?
      include: [/ffmpeg-experiment/],
    },
  },
  worker: {
    // workaround for iife worker bug with `?url` https://github.com/vitejs/vite/issues/9879
    format: "es",
  },
  server: {
    headers: {
      "cross-origin-opener-policy": "same-origin",
      "cross-origin-embedder-policy": "require-corp",
    },
  },
});
