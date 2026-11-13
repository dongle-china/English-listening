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

export interface Page<T> {
  items: T[];
  meta: Pagination;
}
