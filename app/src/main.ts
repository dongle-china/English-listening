import { createApp } from "vue";
import { createPinia } from "pinia";
import App from "./App.vue";
import "./styles/design-system.css";

createApp(App).use(createPinia()).mount("#app");
