import { contextBridge, ipcRenderer } from "electron";

export interface BackendEndpoint {
  httpUrl: string;
  wsUrl: string;
  token: string;
  version: string;
  pid: number;
}

const api = {
  getEndpoint: (): Promise<BackendEndpoint | null> => ipcRenderer.invoke("backend:endpoint"),
  onEndpoint: (callback: (endpoint: BackendEndpoint) => void): (() => void) => {
    const listener = (_event: unknown, endpoint: BackendEndpoint): void => {
      callback(endpoint);
    };
    ipcRenderer.on("backend:endpoint", listener);
    return () => {
      ipcRenderer.removeListener("backend:endpoint", listener);
    };
  },
  onExit: (callback: (code: number | null) => void): (() => void) => {
    const listener = (_event: unknown, code: number | null): void => {
      callback(code);
    };
    ipcRenderer.on("backend:exit", listener);
    return () => {
      ipcRenderer.removeListener("backend:exit", listener);
    };
  },
  onLog: (callback: (line: string) => void): (() => void) => {
    const listener = (_event: unknown, line: string): void => {
      callback(line);
    };
    ipcRenderer.on("backend:log", listener);
    return () => {
      ipcRenderer.removeListener("backend:log", listener);
    };
  },
};

export type ElBackendApi = typeof api;

contextBridge.exposeInMainWorld("elBackend", api);
