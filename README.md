# English Listening

面向英语听写训练的桌面应用。当前仓库包含 **C++17 本地服务后端骨架**；Electron + Vue 3 + TypeScript 渲染端按轮次迁移中。

后端以独立进程运行，由 Electron 主进程 `spawn` 拉起，通过 `stdout` 输出端口与访问令牌完成握手；前端经 REST 与 WebSocket 访问本地服务。播放控制、字幕同步等低延迟能力在渲染端用 Web Audio API 实现，C++ 侧只承担数据、持久化与重计算。

## 当前状态

后端骨架（第一轮）：可编译、可启动、可冒烟。已落地：

- CMake + CMakePresets（MSYS2 UCRT64 / Ninja / C++17）
- 配置集中管理与启动时校验（快速失败）
- 类型化错误体系、全局错误处理、结构化 JSON 日志与请求 ID
- SQLite RAII 访问层与可回滚迁移
- HTTP 健康检查、就绪检查与一个分页示例接口
- 端口与令牌的 stdout 握手
- WebSocket 通道与耗时任务进度推送骨架
- 分层结构：控制器 / 服务 / 仓储

尚未实现：词库完整业务、评分与波形峰值计算、语音识别、云同步、渲染端界面。相关位置标注为待确认，未伪造接口。

## 架构

```
Electron main process
  └─ spawn → el-backend (C++17)
        ├─ stdout: {"event":"ready","port":...,"wsPort":...,"token":...}
        ├─ REST  : cpp-httplib (127.0.0.1:port)
        ├─ WS    : asio standalone (127.0.0.1:wsPort)
        └─ SQLite: data/english_listening.db
```

分层约定：

| 层 | 职责 | 位置 |
| --- | --- | --- |
| Controller | 解析请求、调用服务、格式化响应 | `src/modules/*/*Controller.*` |
| Service | 业务规则与编排，不依赖 HTTP 类型 | `src/modules/*/*Service.*` |
| Repository | 数据查询，参数化 SQL | `src/modules/*/*Repository.*` |
| Core | 配置、错误、日志、握手、HTTP 辅助 | `src/core`, `src/config` |
| Infrastructure | 数据库迁移、WebSocket 广播、路由装配 | `src/db`, `src/ws`, `src/Server.*` |

服务只监听回环地址，不对外暴露。所有请求需携带握手下发的令牌，健康检查端点除外。CORS 使用显式来源，禁止通配符。

## 依赖

| 依赖 | 用途 | 获取方式 |
| --- | --- | --- |
| cpp-httplib | HTTP 服务 | 单头文件，已置于 `third_party/httplib.h` |
| asio (standalone) | WebSocket TCP 层 | MSYS2 `mingw-w64-ucrt-x86_64-asio` |
| nlohmann/json | JSON 序列化 | MSYS2 `mingw-w64-ucrt-x86_64-nlohmann-json` |
| spdlog + fmt | 结构化日志 | MSYS2 `mingw-w64-ucrt-x86_64-spdlog` / `fmt` |
| SQLite3 | 本地持久化 | MSYS2 `mingw-w64-ucrt-x86_64-sqlite` |
| OpenSSL (libcrypto) | WebSocket 握手摘要 | MSYS2 `mingw-w64-ucrt-x86_64-openssl` |

## 构建

需要 CMake 3.25+、Ninja 与 MSYS2 UCRT64 工具链。

```bash
cmake --preset debug
cmake --build --preset debug
```

产物：`build/debug/el-backend.exe`。Release 使用 `--preset release`。

## 运行

通过环境变量配置，参考 `.env.example`：

| 变量 | 默认值 | 说明 |
| --- | --- | --- |
| `EL_HOST` | `127.0.0.1` | 仅允许回环地址 |
| `EL_PORT` | `0` | HTTP 端口，0 表示由系统分配 |
| `EL_DB_PATH` | `./data/english_listening.db` | SQLite 文件 |
| `EL_LOG_LEVEL` | `info` | 日志级别 |
| `EL_CORS_ORIGIN` | `http://localhost:5173` | 显式来源，禁止 `*` |
| `EL_TOKEN` | 自动生成 | 访问令牌，至少 16 字符 |

启动后第一帧 stdout 为握手 JSON，包含 `port`、`wsPort`、`token` 与 `version`。日志输出到 stderr，不污染握手通道。

## 接口（骨架）

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/health` | 进程存活 |
| GET | `/ready` | 数据库迁移完成 |
| GET | `/api/v1/wordbanks` | 词库分页列表，支持 `page` / `pageSize` |
| POST | `/api/v1/wordbanks` | 创建词库 |
| WS | `/ws` | 任务进度事件推送 |

统一响应包含 `ok`、`data`、`requestId`、`ts`；错误响应包含 `ok`、`error.code`、`error.message`、`requestId`、`ts`。时间格式 ISO 8601 UTC。完整接口契约在后续轮次交付。

## 前端路线

Electron + Vite + Vue 3 + TypeScript + Pinia + TailwindCSS。左侧导航、主内容区、可选右侧详情栏，播放器常驻底部带波形与进度；深浅主题令牌、快捷键与无障碍支持按设计系统逐项落地。

## 许可

MIT，见 [LICENSE](LICENSE)。
