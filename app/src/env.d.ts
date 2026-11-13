export interface BackendEndpoint {
  httpUrl: string;
  wsUrl: string;
  token: string;
  version: string;
  pid: number;
}

export interface ElBackendApi {
  getEndpoint: () => Promise<BackendEndpoint | null>;
  onEndpoint: (callback: (endpoint: BackendEndpoint) => void) => () => void;
  onExit: (callback: (code: number | null) => void) => () => void;
  onLog: (callback: (line: string) => void) => () => void;
}

declare global {
  interface Window {
    elBackend: ElBackendApi;
  }
}

export {};
