<script setup lang="ts">
defineProps<{
  shortcuts: ReadonlyArray<{ keys: string; action: string }>;
}>();
</script>

<template>
  <div class="rounded-lg border border-line bg-surface-2 p-4">
    <h3 class="mb-2 text-sm font-semibold text-muted">键盘快捷键</h3>
    <ul class="space-y-1.5">
      <li
        v-for="item in shortcuts"
        :key="item.keys"
        class="flex items-center justify-between text-sm"
      >
        <span class="text-muted">{{ item.action }}</span>
        <kbd
          class="rounded-sm border border-line bg-surface px-2 py-0.5 font-mono text-xs shadow-sm"
        >
          {{ item.keys }}
        </kbd>
      </li>
    </ul>
  </div>
</template>
