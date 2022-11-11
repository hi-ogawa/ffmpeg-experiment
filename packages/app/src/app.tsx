import { useMutation } from "@tanstack/react-query";
import { runFFmpegWorker, WORKER_ASSET_URLS } from "./worker-client";
import { useForm } from "react-hook-form";
import { tinyassert } from "./utils/tinyassert";
import toast from "react-hot-toast";
import TEST_WEBM_URL from "../../../misc/test.webm?url";
import TEST_JPG_URL from "../../../misc/test.jpg?url";
import { GitHub } from "react-feather";
import { Helmet } from "react-helmet-async";
import { AppProvider } from "./app-provider";
import {
  decodeJpeg,
  encodeOpusPicture,
  METADATA_BLOCK_PICTURE,
} from "./utils/opus-picture";
import { toBase64 } from "@hiogawa/base64";
import { booleanGuard } from "./utils/boolean-gurad";

export function App() {
  return (
    <AppProvider>
      <Helmet>
        {WORKER_ASSET_URLS.map((href) => (
          <link key={href} rel="prefetch" href={href} />
        ))}
      </Helmet>
      <AppImpl />
    </AppProvider>
  );
}

interface FormType {
  audioFile?: FileList;
  imageFile?: FileList;
  title: string;
  artist: string;
  album: string;
}

function AppImpl() {
  const processFileMutation = useMutation(processFile, {
    onSuccess: () => {
      toast.success("successfully created an opus file");
    },
    onError: () => {
      toast.error("failed to create an opus file", {
        id: "processFileMutation:onError",
      });
    },
  });

  const form = useForm<FormType>({
    defaultValues: {
      title: "",
      artist: "",
      album: "",
    },
  });
  const { audioFile } = form.watch();

  return (
    <div className="h-full flex flex-col items-center bg-gray-50">
      <div className="w-2xl max-w-full flex flex-col gap-4 p-4">
        <div className="flex items-center gap-3">
          <h1 className="text-2xl">FFmpeg Demo</h1>
          <a
            className="flex items-center"
            href="https://github.com/hi-ogawa/ffmpeg-experiment/blob/main/packages/app"
            target="_blank"
          >
            <GitHub className="w-6 h-6" />
          </a>
          <span className="flex-1"></span>
        </div>
        <div className="flex flex-col gap-2">
          <span className="flex items-baseline gap-2">
            <span>Audio</span>
            <a
              className="text-sm text-blue-500 hover:underline"
              href={TEST_WEBM_URL}
              download="example.webm"
            >
              (example.webm)
            </a>
          </span>
          <input type="file" {...form.register("audioFile")} />
        </div>
        <div className="flex flex-col gap-2">
          <span className="flex items-baseline gap-2">
            <span>Cover Art</span>
            <a
              className="text-sm text-blue-500 hover:underline"
              href={TEST_JPG_URL}
              download="example.jpg"
            >
              (example.jpg)
            </a>
          </span>
          <input type="file" {...form.register("imageFile")} />
        </div>
        <div className="flex flex-col gap-2">
          <span>Title</span>
          <input className="border" {...form.register("title")} />
        </div>
        <div className="flex flex-col gap-2">
          <span>Artist</span>
          <input className="border" {...form.register("artist")} />
        </div>
        <div className="flex flex-col gap-2">
          <span>Album</span>
          <input className="border" {...form.register("album")} />
        </div>
        <button
          className="p-1 border bg-gray-300 filter transition duration-200 hover:brightness-95 disabled:(pointer-events-none text-gray-500 bg-gray-200)"
          disabled={!audioFile?.length || processFileMutation.isLoading}
          onClick={form.handleSubmit((data) => {
            processFileMutation.mutate(data);
          })}
        >
          <div className="flex justify-center items-center relative">
            <span>Create Opus File</span>
            {processFileMutation.isLoading && (
              <div className="absolute right-2 w-5 h-5 spinner"></div>
            )}
          </div>
        </button>
        <a
          className="p-1 border bg-gray-300 filter transition duration-200 hover:brightness-95 aria-disabled:(pointer-events-none text-gray-500 bg-gray-200) text-center cursor"
          aria-disabled={!processFileMutation.isSuccess}
          href={
            processFileMutation.isSuccess
              ? processFileMutation.data.url
              : undefined
          }
          download={
            processFileMutation.isSuccess
              ? processFileMutation.data.name
              : undefined
          }
        >
          Download
        </a>
      </div>
    </div>
  );
}

async function processFile(
  data: FormType
): Promise<{ url: string; name: string }> {
  const audio = data.audioFile?.[0];
  tinyassert(audio);
  const audioData = new Uint8Array(await audio.arrayBuffer());

  const image = data.imageFile?.[0];
  const encodedImage =
    image && processImage(new Uint8Array(await image.arrayBuffer()));

  const IN_FILE = "/in.webm";
  const OUT_FILE = "/out.opus";
  const result = await runFFmpegWorker({
    // prettier-ignore
    arguments: [
      "-hide_banner",
      "-i", IN_FILE,
      "-c", "copy",
      data.artist && ["-metadata", `artist=${data.artist}`],
      data.title  && ["-metadata", `title=${data.title}`],
      encodedImage && ["-metadata", `${METADATA_BLOCK_PICTURE}=${encodedImage}`],
      OUT_FILE
    ].filter(booleanGuard).flat(),
    inFiles: [
      {
        path: IN_FILE,
        data: audioData,
      },
    ],
    outFiles: [
      {
        path: OUT_FILE,
      },
    ],
    onStdout: (data) => console.log({ stdout: data }),
    onStderr: (data) => console.log({ stderr: data }),
  });
  tinyassert(result.exitCode === 0);

  const outFile = result.outFiles.find((f) => f.path === OUT_FILE);
  tinyassert(outFile);

  // TODO: URL.revokeObjectURL
  const url = URL.createObjectURL(new Blob([outFile.data]));
  const name =
    ([data.artist, data.album, data.title].filter(Boolean).join(" - ") ||
      "download") + ".opus";
  return { url, name };
}

function processImage(data: Uint8Array): string {
  const info = decodeJpeg(data);
  const encodedBin = encodeOpusPicture(data, info, 3, ""); // 3 means "cover art"
  const encodedBase64Bin = toBase64(encodedBin);
  return new TextDecoder().decode(encodedBase64Bin);
}
