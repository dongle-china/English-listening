export interface WordBank {
  id: string;
  name: string;
  description: string;
  wordCount: number;
  createdAt: string;
  updatedAt: string;
}

export interface Pagination {
  page: number;
  pageSize: number;
  total: number;
}

export interface PageData<T> {
  items: T[];
}

export function readPagination(meta: Record<string, unknown> | undefined): Pagination {
  return {
    page: typeof meta?.["page"] === "number" ? meta["page"] : 1,
    pageSize: typeof meta?.["pageSize"] === "number" ? meta["pageSize"] : 20,
    total: typeof meta?.["total"] === "number" ? meta["total"] : 0,
  };
}
