<script setup lang="ts">
import { onBeforeUnmount, onMounted, ref } from "vue";
import { useBackendStore } from "@/stores/backend";
import { useSettingsStore } from "@/stores/settings";
import { useToastStore } from "@/stores/toast";
import AppSidebar from "@/components/AppSidebar.vue";
import ToastHost from "@/components/ToastHost.vue";
import WordBankView from "@/pages/WordBankView.vue";

const backend = useBackendStore();
const settings = useSettingsStore();
const toastStore = useToastStore();
const activePage = ref<"wordbank">("wordbank");

const unsubs: Array<() => void> = [];

onMounted(() => {
  settings.applyTheme();
  void window.elBackend.getEndpoint().then((endpoint) => {
    if (endpoint !== null) backend.adopt(endpoint);
    else if (backend.state === "connecting") toastStore.push("info", "正在等待本地服务启动…");
  });
  unsubs.push(
    window.elBackend.onEndpoint((endpoint) => {
      backend.adopt(endpoint);
      toastStore.push("success", `本地服务已连接 v${endpoint.version}`);
    }),
  );
  unsubs.push(
    window.elBackend.onExit((code) => {
      backend.markStopped(code);
      toastStore.push("error", `本地服务已退出（code=${String(code)}）`);
    }),
  );
  unsubs.push(
    window.elBackend.onLog((line) => {
      backend.lastLog = line;
    }),
  );
});

onBeforeUnmount(() => {
  for (const unsub of unsubs) unsub();
});
</script>

<template>
  <div class="flex h-screen w-screen overflow-hidden bg-bg text-text">
    <AppSidebar />
    <main class="flex min-w-0 flex-1 flex-col">
      <div class="flex-1 overflow-y-auto">
        <WordBankView />
      </div>
      <footer
        class="flex h-16 shrink-0 items-center justify-between border-t border-line bg-surface px-6 text-sm text-muted"
        aria-label="播放器"
      >
        <span>播放器占位 · 波形与进度随后续轮次接入</span>
        <span class="font-mono text-xs text-faint">
          {{ backend.endpoint !== null ? `${backend.endpoint.httpUrl}` : "not connected" }}
        </span>
      </footer>
    </main>
    <ToastHost />
  </div>
</template>
