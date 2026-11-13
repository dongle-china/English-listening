<script setup lang="ts">
import { useBackendStore } from "@/stores/backend";

const backend = useBackendStore();

const items = [
  { key: "home", label: "主页", enabled: false },
  { key: "wordbank", label: "词库", enabled: true },
  { key: "dictation", label: "听写", enabled: false },
  { key: "memorize", label: "背单词", enabled: false },
  { key: "progress", label: "学习进度", enabled: false },
] as const;
</script>

<template>
  <nav class="flex h-full w-sidebar flex-col border-r border-line bg-surface" aria-label="主导航">
    <div class="flex items-center gap-2 px-4 py-4">
      <span class="inline-block h-2.5 w-2.5 rounded-full bg-accent" aria-hidden="true"></span>
      <span class="text-lg font-semibold tracking-tight">English Listen</span>
    </div>

    <ul class="flex-1 space-y-1 px-2">
      <li v-for="item in items" :key="item.key">
        <button
          type="button"
          class="w-full rounded-md px-3 py-2 text-left text-sm transition-colors duration-fast ease-standard"
          :class="item.enabled
            ? 'bg-accent-soft text-text hover:bg-surface-hover'
            : 'cursor-not-allowed text-faint'"
          :disabled="!item.enabled"
          :aria-disabled="!item.enabled"
        >
          {{ item.label }}
        </button>
      </li>
    </ul>

    <div class="border-t border-line px-4 py-3 text-xs text-muted">
      <div class="flex items-center gap-2">
        <span
          class="inline-block h-2 w-2 rounded-full"
          :class="{
            'bg-success animate-pulse': backend.state === 'ready',
            'bg-warning animate-pulse': backend.state === 'connecting',
            'bg-danger': backend.state === 'failed' || backend.state === 'stopped',
          }"
          aria-hidden="true"
        ></span>
        <span>
          {{
            backend.state === "ready"
              ? `服务已连接 v${backend.version}`
              : backend.state === "connecting"
                ? "正在连接本地服务…"
                : backend.state === "failed"
                  ? "本地服务启动失败"
                  : "本地服务已停止"
          }}
        </span>
      </div>
    </div>
  </nav>
</template>
