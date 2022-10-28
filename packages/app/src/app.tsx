import React from "react";
import {
  QueryClient,
  QueryClientProvider,
  useMutation,
  useQuery,
} from "@tanstack/react-query";
import { ReactQueryDevtools } from "@tanstack/react-query-devtools";
import { getWorker } from "./worker-client";
import { useForm } from "react-hook-form";
import { tinyassert } from "./utils/tinyassert";
import toast, { Toaster } from "react-hot-toast";
import type { WorkerImpl } from "./worker-impl";
import type { Remote } from "comlink";
import TEST_WEBM_URL from "../../../misc/test.webm?url";
import TEST_JPG_URL from "../../../misc/test.jpg?url";

export function App() {
  return (
    <Providers>
      <AppImpl />
      <Toaster />
    </Providers>
  );
}

function Providers(props: React.PropsWithChildren) {
  const [queryClient] = React.useState(
    () =>
      new QueryClient({
        defaultOptions: {
          queries: {
            refetchOnWindowFocus: false,
            refetchOnReconnect: false,
            retry: 0,
          },
        },
      })
  );
  return (
    <QueryClientProvider client={queryClient}>
      {props.children}
      {import.meta.env.DEV && <ReactQueryDevtools />}
    </QueryClientProvider>
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
  const workerQuery = useWorker();

  const processFileMutation = useMutation(
    async (data: FormType) => {
      tinyassert(workerQuery.isSuccess);
      return processFile(workerQuery.data, data);
    },
    {
      onSuccess: () => {
        toast.success("successfully created an opus file");
      },
      onError: () => {
        toast.error("failed to create an opus file", {
          id: "processFileMutation:onError",
        });
      },
    }
  );

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
        <div className="flex items-center gap-4">
          <h1 className="text-2xl">FFmpeg Demo</h1>
          <span className="flex-1"></span>
          {workerQuery.isLoading && (
            <div className="w-6 h-6 spinner" title="loading wasm..." />
          )}
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
          disabled={
            !workerQuery.isSuccess ||
            !audioFile?.length ||
            processFileMutation.isLoading
          }
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

function useWorker() {
  return useQuery({
    queryKey: [useWorker.name],
    queryFn: () => getWorker(),
    staleTime: Infinity,
    cacheTime: Infinity,
    onError: () => {
      toast.error("failed to load wasm");
    },
  });
}

async function processFile(
  remoteWorker: Remote<WorkerImpl>,
  data: FormType
): Promise<{ url: string; name: string }> {
  const audio = data.audioFile?.[0];
  tinyassert(audio);
  const audioData = new Uint8Array(await audio.arrayBuffer());

  const image = data.imageFile?.[0];
  const imageData = image && new Uint8Array(await image.arrayBuffer());

  const output = await remoteWorker.convert({
    data: audioData,
    outFormat: "opus",
    picture: imageData,
    metadata: {
      title: data.title,
      artist: data.artist,
      album: data.album,
    },
  });
  // TODO: URL.revokeObjectURL
  const url = URL.createObjectURL(new Blob([output]));
  const name =
    ([data.artist, data.album, data.title].filter(Boolean).join(" / ") ||
      "download") + ".opus";
  return { url, name };
}
