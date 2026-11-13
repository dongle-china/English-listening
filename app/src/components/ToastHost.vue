<script setup lang="ts">
import { useToastStore } from "@/stores/toast";

const toastStore = useToastStore();
</script>

<template>
  <div
    class="pointer-events-none fixed right-4 top-4 z-50 flex w-80 flex-col gap-2"
    role="status"
    aria-live="polite"
  >
    <TransitionGroup
      enter-active-class="transition duration-normal ease-out-quart"
      enter-from-class="translate-x-4 opacity-0"
      leave-active-class="transition duration-fast ease-standard"
      leave-to-class="opacity-0"
    >
      <div
        v-for="toast in toastStore.toasts"
        :key="toast.id"
        class="pointer-events-auto flex items-start justify-between gap-3 rounded-md border border-line bg-surface-2 px-4 py-3 shadow-md"
        :class="{
          'border-l-4 border-l-success': toast.kind === 'success',
          'border-l-4 border-l-danger': toast.kind === 'error',
          'border-l-4 border-l-accent': toast.kind === 'info',
        }"
      >
        <span class="text-sm">{{ toast.message }}</span>
        <button
          type="button"
          class="text-muted transition-colors duration-fast hover:text-text"
          aria-label="关闭提示"
          @click="toastStore.dismiss(toast.id)"
        >
          ×
        </button>
      </div>
    </TransitionGroup>
  </div>
</template>
