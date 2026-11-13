import { defineStore } from "pinia";
import { ref } from "vue";

export interface Toast {
  id: number;
  kind: "info" | "success" | "error";
  message: string;
}

let sequence = 0;

export const useToastStore = defineStore("toast", () => {
  const toasts = ref<Toast[]>([]);

  function push(kind: Toast["kind"], message: string, ttlMs = 4000): void {
    sequence += 1;
    const id = sequence;
    toasts.value.push({ id, kind, message });
    setTimeout(() => {
      dismiss(id);
    }, ttlMs);
  }

  function dismiss(id: number): void {
    toasts.value = toasts.value.filter((toast) => toast.id !== id);
  }

  return { toasts, push, dismiss };
});
