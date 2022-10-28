import react from "@vitejs/plugin-react";
import { defineConfig } from "vite";
import windicss from "vite-plugin-windicss";

export default defineConfig({
  base: "./",
  plugins: [windicss(), react()],
  optimizeDeps: {
    include: ["@hiogawa/ffmpeg-experiment"],
  },
  worker: {
    // workaround for iife worker bug with `?url` https://github.com/vitejs/vite/issues/9879
    format: "es",
  },
});
