import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import { ReactQueryDevtools } from "@tanstack/react-query-devtools";
import React from "react";
import { HelmetProvider } from "react-helmet-async";
import { Toaster } from "react-hot-toast";

export function AppProvider(props: React.PropsWithChildren) {
  let node = props.children;
  for (const Provider of PROVIDERS) {
    node = <Provider>{node}</Provider>;
  }
  return <>{node}</>;
}

const PROVIDERS = [ToastProvider, HelmetProvider, CustomQueryClientProvider];

function CustomQueryClientProvider(props: React.PropsWithChildren) {
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

function ToastProvider(props: React.PropsWithChildren) {
  return (
    <>
      <Toaster />
      {props.children}
    </>
  );
}
