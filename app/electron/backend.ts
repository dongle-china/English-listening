import { spawn, type ChildProcess } from "node:child_process";
import { EventEmitter } from "node:events";
import { createInterface } from "node:readline";
import { existsSync } from "node:fs";
import path from "node:path";

export interface BackendHandshake {
  event: "ready";
  host: string;
  pid: number;
  port: number;
  token: string;
  ts: string;
  version: string;
  wsPort: number;
}

export interface BackendEndpoint {
  httpUrl: string;
  wsUrl: string;
  token: string;
  version: string;
  pid: number;
}

const HANDSHAKE_TIMEOUT_MS = 15_000;

function resolveBackendBinary(): string {
  const override = process.env.EL_BACKEND_BIN;
  if (override !== undefined && override.trim().length > 0 && existsSync(override)) {
    return path.resolve(override);
  }
  const candidates =
    process.env.NODE_ENV === "production"
      ? [
          path.join(process.resourcesPath ?? "", "backend", "el-backend.exe"),
          path.join(process.resourcesPath ?? "", "backend", "el-backend"),
        ]
      : [
          path.join(process.cwd(), "..", "build", "debug", "el-backend.exe"),
          path.join(process.cwd(), "..", "build", "release", "el-backend.exe"),
        ];
  for (const candidate of candidates) {
    if (candidate.length > 0 && existsSync(candidate)) return path.resolve(candidate);
  }
  throw new Error(
    `backend binary not found (set EL_BACKEND_BIN); tried: ${candidates.filter((c) => c.length > 0).join(", ")}`,
  );
}

function parseHandshakeLine(line: string): BackendHandshake | null {
  const trimmed = line.trim();
  if (!trimmed.startsWith("{")) return null;
  try {
    const parsed: unknown = JSON.parse(trimmed);
    if (typeof parsed !== "object" || parsed === null) return null;
    const record = parsed as Record<string, unknown>;
    if (
      record["event"] === "ready" &&
      typeof record["port"] === "number" &&
      typeof record["wsPort"] === "number" &&
      typeof record["token"] === "string" &&
      typeof record["host"] === "string" &&
      typeof record["version"] === "string" &&
      typeof record["pid"] === "number"
    ) {
      return parsed as BackendHandshake;
    }
    return null;
  } catch {
    return null;
  }
}

export class BackendProcess extends EventEmitter {
  private child: ChildProcess | null = null;

  override emit<EventKey extends keyof BackendEventsMap>(
    event: EventKey,
    ...args: Parameters<BackendEventsMap[EventKey]>
  ): boolean {
    return super.emit(event, ...args);
  }

  override on<EventKey extends keyof BackendEventsMap>(
    event: EventKey,
    listener: BackendEventsMap[EventKey],
  ): this {
    return super.on(event, listener);
  }

  start(): void {
    if (this.child !== null) return;
    const binary = resolveBackendBinary();
    const child = spawn(binary, [], {
      stdio: ["ignore", "pipe", "pipe"],
      windowsHide: true,
    });
    this.child = child;

    let settled = false;
    const timer = setTimeout(() => {
      if (settled) return;
      settled = true;
      this.emit("log", "[backend] handshake timeout");
      this.stop();
    }, HANDSHAKE_TIMEOUT_MS);

    if (child.stdout !== null) {
      const stdoutLines = createInterface({ input: child.stdout });
      stdoutLines.on("line", (line: string) => {
        const handshake = parseHandshakeLine(line);
        if (handshake === null || settled) return;
        settled = true;
        clearTimeout(timer);
        this.emit("ready", {
          httpUrl: `http://${handshake.host}:${handshake.port}`,
          wsUrl: `ws://${handshake.host}:${handshake.wsPort}/ws`,
          token: handshake.token,
          version: handshake.version,
          pid: handshake.pid,
        } satisfies BackendEndpoint);
      });
    }
    if (child.stderr !== null) {
      const stderrLines = createInterface({ input: child.stderr });
      stderrLines.on("line", (line: string) => {
        this.emit("log", line);
      });
    }
    child.once("exit", (code) => {
      clearTimeout(timer);
      this.child = null;
      this.emit("exit", code);
    });
  }

  stop(): void {
    const child = this.child;
    if (child === null || child.exitCode !== null) return;
    child.kill();
    const forceKill = setTimeout(() => {
      if (child.exitCode === null) child.kill("SIGKILL");
    }, 3000);
    child.once("exit", () => {
      clearTimeout(forceKill);
      this.child = null;
    });
  }
}

export interface BackendEventsMap {
  ready: (endpoint: BackendEndpoint) => void;
  exit: (code: number | null) => void;
  log: (line: string) => void;
}
