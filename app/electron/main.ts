import { app, BrowserWindow, ipcMain } from "electron";
import path from "node:path";
import { BackendProcess, type BackendEndpoint } from "./backend";

let mainWindow: BrowserWindow | null = null;
let backend: BackendProcess | null = null;
let endpoint: BackendEndpoint | null = null;

const isDev = process.env.NODE_ENV !== "production";
const devServerUrl = process.env.VITE_DEV_SERVER_URL;

function createWindow(): void {
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 820,
    minWidth: 960,
    minHeight: 640,
    backgroundColor: "#171717",
    show: false,
    autoHideMenuBar: true,
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: false,
    },
  });

  mainWindow.once("ready-to-show", () => {
    mainWindow?.show();
  });

  if (devServerUrl !== undefined && devServerUrl.length > 0) {
    void mainWindow.loadURL(devServerUrl);
  } else {
    void mainWindow.loadFile(path.join(__dirname, "..", "dist", "index.html"));
  }

  mainWindow.on("closed", () => {
    mainWindow = null;
  });
}

function startBackend(): void {
  backend = new BackendProcess();
  backend.on("ready", (next) => {
    endpoint = next;
    mainWindow?.webContents.send("backend:endpoint", endpoint);
  });
  backend.on("exit", (code) => {
    endpoint = null;
    mainWindow?.webContents.send("backend:exit", code);
  });
  backend.on("log", (line) => {
    mainWindow?.webContents.send("backend:log", line);
  });
  backend.start();
}

ipcMain.handle("backend:endpoint", () => endpoint);

app.whenReady().then(() => {
  startBackend();
  createWindow();
  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") app.quit();
});

app.on("before-quit", () => {
  backend?.stop();
});

process.on("exit", () => {
  backend?.stop();
});

if (isDev) {
  process.on("SIGINT", () => {
    backend?.stop();
    app.quit();
  });
}
