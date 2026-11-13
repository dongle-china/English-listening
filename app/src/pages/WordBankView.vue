<script setup lang="ts">
import { onBeforeUnmount, onMounted, ref } from "vue";
import { storeToRefs } from "pinia";
import { useBackendStore } from "@/stores/backend";
import { useToastStore } from "@/stores/toast";
import { useSettingsStore } from "@/stores/settings";
import { ApiError, NetworkError } from "@/api/client";
import type { Page, WordBank } from "@/api/types";
import ShortcutsCard from "@/components/ShortcutsCard.vue";

const backend = useBackendStore();
const toastStore = useToastStore();
const settings = useSettingsStore();

const page = ref(1);
const pageSize = ref(20);
const items = ref<WordBank[]>([]);
const total = ref(0);
const loading = ref(false);
const loadError = ref("");
const newName = ref("");
const newDescription = ref("");
const creating = ref(false);
const selectedIndex = ref(-1);

const shortcuts = [
  { keys: "Space", action: "播放 / 暂停（播放器就绪后生效）" },
  { keys: "←", action: "快退 5 秒" },
  { keys: "→", action: "快进 5 秒" },
] as const;

function friendlyError(error: unknown): string {
  if (error instanceof NetworkError) return error.message;
  if (error instanceof ApiError) return `${error.message}（${error.code}）`;
  return error instanceof Error ? error.message : String(error);
}

async function load(): Promise<void> {
  loading.value = true;
  loadError.value = "";
  try {
    const result = await backend.client.get<Page<WordBank>>(
      `/api/v1/wordbanks?page=${String(page.value)}&pageSize=${String(pageSize.value)}`,
    );
    items.value = result.data.items;
    total.value = result.data.meta.total;
    if (selectedIndex.value >= items.value.length) selectedIndex.value = -1;
  } catch (error) {
    loadError.value = friendlyError(error);
  } finally {
    loading.value = false;
  }
}

async function create(): Promise<void> {
  if (newName.value.trim().length === 0) {
    toastStore.push("error", "词库名称不能为空");
    return;
  }
  creating.value = true;
  try {
    await backend.client.post<WordBank>("/api/v1/wordbanks", {
      name: newName.value.trim(),
      description: newDescription.value.trim(),
    });
    toastStore.push("success", `词库「${newName.value.trim()}」已创建`);
    newName.value = "";
    newDescription.value = "";
    page.value = 1;
    await load();
  } catch (error) {
    toastStore.push("error", friendlyError(error));
  } finally {
    creating.value = false;
  }
}

async function remove(item: WordBank): Promise<void> {
  try {
    await backend.client.delete(`/api/v1/wordbanks/${item.id}`);
    toastStore.push("success", `词库「${item.name}」已删除`);
    await load();
  } catch (error) {
    toastStore.push("error", friendlyError(error));
  }
}

function goToPage(next: number): void {
  const maxPage = Math.max(1, Math.ceil(total.value / pageSize.value));
  page.value = Math.min(Math.max(1, next), maxPage);
  void load();
}

function onKeydown(event: KeyboardEvent): void {
  if (event.key === "ArrowLeft") toastStore.push("info", "← 快退（播放器未就绪）");
  if (event.key === "ArrowRight") toastStore.push("info", "→ 快进（播放器未就绪）");
}

onMounted(() => {
  void load();
  window.addEventListener("keydown", onKeydown);
});

onBeforeUnmount(() => {
  window.removeEventListener("keydown", onKeydown);
});

const { state } = storeToRefs(backend);
</script>

<template>
  <section class="mx-auto w-full max-w-5xl px-6 py-6">
    <header class="mb-6 flex items-baseline justify-between">
      <h1 class="text-2xl font-semibold tracking-tight">词库</h1>
      <span class="text-sm text-muted">共 {{ String(total) }} 个词库</span>
    </header>

    <form
      class="mb-6 flex flex-wrap items-end gap-3 rounded-lg border border-line bg-surface p-4"
      @submit.prevent="create"
    >
      <div class="flex min-w-52 flex-1 flex-col gap-1">
        <label class="text-xs font-medium text-muted" for="wb-name">词库名称</label>
        <input
          id="wb-name"
          v-model="newName"
          type="text"
          maxlength="128"
          placeholder="例如：CET-4 核心词"
          class="rounded-md border border-line bg-surface-2 px-3 py-2 text-sm transition-colors duration-fast focus:border-accent"
        />
      </div>
      <div class="flex min-w-52 flex-1 flex-col gap-1">
        <label class="text-xs font-medium text-muted" for="wb-desc">描述（可选）</label>
        <input
          id="wb-desc"
          v-model="newDescription"
          type="text"
          maxlength="1024"
          placeholder="一句话说明"
          class="rounded-md border border-line bg-surface-2 px-3 py-2 text-sm transition-colors duration-fast focus:border-accent"
        />
      </div>
      <button
        type="submit"
        :disabled="creating"
        class="rounded-md bg-accent px-4 py-2 text-sm font-medium text-white shadow-sm transition-colors duration-fast hover:bg-accent-hover disabled:opacity-50"
      >
        {{ creating ? "创建中…" : "新建词库" }}
      </button>
    </form>

    <div v-if="state !== 'ready'" class="rounded-lg border border-line bg-surface-2 p-8 text-center text-muted">
      本地服务尚未就绪，无法读取词库。
    </div>

    <div v-else-if="loading" class="space-y-3" aria-busy="true" aria-label="加载中">
      <div
        v-for="index in 4"
        :key="index"
        class="h-14 animate-pulse rounded-lg bg-surface-2"
      ></div>
    </div>

    <div v-else-if="loadError.length > 0" class="rounded-lg border border-line bg-surface-2 p-8 text-center">
      <p class="mb-3 text-danger">{{ loadError }}</p>
      <button
        type="button"
        class="rounded-md border border-line px-4 py-2 text-sm transition-colors duration-fast hover:bg-surface-hover"
        @click="load"
      >
        重试
      </button>
    </div>

    <div v-else-if="items.length === 0" class="rounded-lg border border-line bg-surface-2 p-10 text-center">
      <p class="mb-1 font-medium">还没有词库</p>
      <p class="text-sm text-muted">用上方表单创建第一个词库，开始听写训练。</p>
    </div>

    <ul v-else class="space-y-3">
      <li
        v-for="(item, index) in items"
        :key="item.id"
        class="flex items-center justify-between rounded-lg border border-line bg-surface px-4 py-3 shadow-sm transition-colors duration-fast hover:bg-surface-hover"
        @click="selectedIndex = index"
      >
        <div class="min-w-0">
          <p class="truncate font-medium">{{ item.name }}</p>
          <p class="truncate text-sm text-muted">
            {{ item.description.length > 0 ? item.description : "暂无描述" }}
          </p>
        </div>
        <div class="ml-4 flex shrink-0 items-center gap-3 text-sm">
          <span class="text-muted">{{ String(item.wordCount) }} 词</span>
          <button
            type="button"
            class="rounded-sm px-2 py-1 text-danger transition-colors duration-fast hover:bg-accent-soft"
            :aria-label="`删除词库 ${item.name}`"
            @click.stop="remove(item)"
          >
            删除
          </button>
        </div>
      </li>
    </ul>

    <nav
      v-if="total > pageSize"
      class="mt-6 flex items-center justify-center gap-3 text-sm"
      aria-label="分页"
    >
      <button
        type="button"
        class="rounded-md border border-line px-3 py-1.5 transition-colors duration-fast hover:bg-surface-hover"
        :disabled="page <= 1"
        @click="goToPage(page - 1)"
      >
        上一页
      </button>
      <span class="text-muted">第 {{ String(page) }} 页</span>
      <button
        type="button"
        class="rounded-md border border-line px-3 py-1.5 transition-colors duration-fast hover:bg-surface-hover"
        :disabled="page >= Math.ceil(total / pageSize)"
        @click="goToPage(page + 1)"
      >
        下一页
      </button>
    </nav>

    <div class="mt-8 flex items-start justify-between gap-6">
      <ShortcutsCard :shortcuts="shortcuts" />
      <div class="flex items-center gap-3 text-sm text-muted">
        <span>主题</span>
        <button
          type="button"
          class="rounded-md border border-line px-3 py-1.5 transition-colors duration-fast hover:bg-surface-hover"
          @click="settings.setTheme(settings.theme === 'dark' ? 'light' : 'dark')"
        >
          切换到{{ settings.theme === "dark" ? "浅色" : "深色" }}
        </button>
        <button
          type="button"
          class="rounded-md border border-line px-3 py-1.5 transition-colors duration-fast hover:bg-surface-hover"
          @click="settings.setFontScale(settings.fontScale === 1 ? 1.2 : 1)"
        >
          {{ settings.fontScale === 1 ? "放大字体" : "恢复字体" }}
        </button>
      </div>
    </div>
  </section>
</template>
