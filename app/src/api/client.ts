export class ApiError extends Error {
  readonly code: string;
  readonly status: number;
  readonly requestId: string;

  constructor(code: string, message: string, status: number, requestId: string) {
    super(message);
    this.name = "ApiError";
    this.code = code;
    this.status = status;
    this.requestId = requestId;
  }
}

export class NetworkError extends Error {
  constructor() {
    super("网络不可用，请检查本地服务是否运行");
    this.name = "NetworkError";
  }
}

interface EnvelopeBase {
  ok: boolean;
  requestId: string;
  ts: string;
}

interface SuccessEnvelope<T> extends EnvelopeBase {
  ok: true;
  data: T;
  meta?: Record<string, unknown>;
}

interface ErrorEnvelope extends EnvelopeBase {
  ok: false;
  error: {
    code: string;
    message: string;
    details?: Record<string, unknown>;
  };
}

type Envelope<T> = SuccessEnvelope<T> | ErrorEnvelope;

const MAX_RETRIES = 3;

function isRetryable(status: number): boolean {
  return status >= 500 || status === 0;
}

const sleep = (ms: number): Promise<void> => new Promise((resolve) => setTimeout(resolve, ms));

export interface ApiClientConfig {
  baseUrl: string;
  token: string;
}

export class ApiClient {
  private config: ApiClientConfig;

  constructor(config: ApiClientConfig) {
    this.config = config;
  }

  update(config: ApiClientConfig): void {
    this.config = config;
  }

  private async request<T>(method: string, path: string, body?: unknown): Promise<{ data: T; meta: Record<string, unknown> | undefined; requestId: string }> {
    if (this.config.baseUrl.length === 0 || this.config.token.length === 0) {
      throw new NetworkError();
    }
    const url = `${this.config.baseUrl}${path}`;
    let lastError: Error = new NetworkError();

    for (let attempt = 0; attempt <= MAX_RETRIES; attempt += 1) {
      let response: Response;
      try {
        response = await fetch(url, {
          method,
          headers: {
            Authorization: `Bearer ${this.config.token}`,
            ...(body === undefined ? {} : { "Content-Type": "application/json" }),
          },
          body: body === undefined ? undefined : JSON.stringify(body),
        });
      } catch {
        lastError = new NetworkError();
        if (attempt < MAX_RETRIES) {
          await sleep(200 * (attempt + 1));
          continue;
        }
        throw lastError;
      }

      let envelope: Envelope<T> | null = null;
      try {
        envelope = (await response.json()) as Envelope<T>;
      } catch {
        envelope = null;
      }

      if (envelope !== null && envelope.ok) {
        return {
          data: envelope.data,
          meta: envelope.meta,
          requestId: envelope.requestId,
        };
      }

      if (envelope !== null && !envelope.ok) {
        lastError = new ApiError(
          envelope.error.code,
          envelope.error.message,
          response.status,
          envelope.requestId,
        );
        if (isRetryable(response.status) && attempt < MAX_RETRIES) {
          await sleep(200 * (attempt + 1));
          continue;
        }
        throw lastError;
      }

      lastError = new ApiError("INVALID_RESPONSE", `unexpected response (HTTP ${String(response.status)})`, response.status, "");
      if (isRetryable(response.status) && attempt < MAX_RETRIES) {
        await sleep(200 * (attempt + 1));
        continue;
      }
      throw lastError;
    }
    throw lastError;
  }

  async get<T>(path: string): Promise<{ data: T; meta: Record<string, unknown> | undefined; requestId: string }> {
    return this.request<T>("GET", path);
  }

  async post<T>(path: string, body: unknown): Promise<{ data: T; meta: Record<string, unknown> | undefined; requestId: string }> {
    return this.request<T>("POST", path, body);
  }

  async delete<T>(path: string): Promise<{ data: T; meta: Record<string, unknown> | undefined; requestId: string }> {
    return this.request<T>("DELETE", path);
  }
}
