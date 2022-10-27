import React from "react";
import {
  QueryClient,
  QueryClientProvider,
  useQuery,
} from "@tanstack/react-query";
import { ReactQueryDevtools } from "@tanstack/react-query-devtools";

export function App() {
  return (
    <Providers>
      <AppImpl />
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

function AppImpl() {
  const lib = useWasmModule();

  return (
    <div className="h-full flex flex-col items-center bg-gray-50">
      <div className="w-2xl max-w-full flex flex-col gap-4 p-4">
        <div>hello</div>
      </div>
    </div>
  );
}

import WASM_URL from "@hiogawa/ffmpeg-experiment/build/index.wasm?url";

function useWasmModule() {
  useQuery({
    queryKey: ["useWasmModule"],
    queryFn: async () => {
      const { default: Module } = await import("@hiogawa/ffmpeg-experiment");
      return Module({ locateFile: () => WASM_URL });
    },
    staleTime: Infinity,
    cacheTime: Infinity,
  });
}
