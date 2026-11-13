import { defineStore } from "pinia";
import { ref, shallowRef } from "vue";
import { ApiClient } from "@/api/client";
import type { BackendEndpoint } from "@/env.d";

export type BackendState = "connecting" | "ready" | "stopped" | "failed";

export const useBackendStore = defineStore("backend", () => {
  const state = ref<BackendState>("connecting");
  const endpoint = shallowRef<BackendEndpoint | null>(null);
  const version = ref("");
  const lastLog = ref("");
  const client = new ApiClient({ baseUrl: "", token: "" });

  function adopt(next: BackendEndpoint): void {
    endpoint.value = next;
    version.value = next.version;
    client.update({ baseUrl: next.httpUrl, token: next.token });
    state.value = "ready";
  }

  function markStopped(code: number | null): void {
    endpoint.value = null;
    client.update({ baseUrl: "", token: "" });
    state.value = code === null || code === 0 ? "stopped" : "failed";
  }

  function markFailed(): void {
    state.value = "failed";
  }

  return { state, endpoint, version, lastLog, client, adopt, markStopped, markFailed };
});
