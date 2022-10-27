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
      include: [/@hiogawa\/ffmpeg-experiment/],
    },
  },
});
