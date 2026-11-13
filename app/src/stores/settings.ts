import { defineStore } from "pinia";
import { ref } from "vue";

export type ThemeMode = "dark" | "light";

export const useSettingsStore = defineStore("settings", () => {
  const theme = ref<ThemeMode>("dark");
  const fontScale = ref(1);

  function applyTheme(): void {
    document.documentElement.dataset["theme"] = theme.value;
    document.documentElement.style.setProperty("--font-scale", String(fontScale.value));
  }

  function setTheme(mode: ThemeMode): void {
    theme.value = mode;
    applyTheme();
  }

  function setFontScale(scale: number): void {
    fontScale.value = Math.min(1.4, Math.max(0.85, scale));
    applyTheme();
  }

  return { theme, fontScale, setTheme, setFontScale, applyTheme };
});
