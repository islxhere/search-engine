# Buugle 搜索客户端

这是一个独立于 C++ 后端的浏览器客户端，支持关键词联想、网页搜索、键盘操作和
可记忆的浅色/深色主题。浏览器只访问本地 HTTP 服务；`bridge.mjs` 以固定白名单提供
`/`、`/index.html`、`/styles.css` 和 `/app.js`，并把两个 API 请求转换成
搜索服务使用的 TLV/TCP 协议。每次 API 请求都会新建一条 TCP 连接，请求完成后关闭。

## 启动

没有搜索服务时，在项目根目录运行：

```bash
MOCK_DATA=1 node client/bridge.mjs
```

打开 `http://127.0.0.1:4173` 后，输入 `人工智能`、`开源搜索` 或
`机器学习` 查看联想词和搜索结果；任意非空关键词也能演示。

连接真实搜索服务时，运行：

```bash
node client/bridge.mjs
```

默认监听地址为 `0.0.0.0:4173`，可从局域网通过本机 IP 访问；搜索服务地址为
`127.0.0.1:8888`。可用以下环境变量覆盖：

| 环境变量 | 默认值 | 用途 |
| --- | --- | --- |
| `CLIENT_HOST` | `0.0.0.0` | HTTP 监听地址 |
| `CLIENT_PORT` | `4173` | HTTP 监听端口 |
| `SEARCH_SERVER_HOST` | `127.0.0.1` | 搜索服务 TCP 地址 |
| `SEARCH_SERVER_PORT` | `8888` | 搜索服务 TCP 端口 |
| `MOCK_DATA` | 未设置 | 设为 `1` 时使用内置演示数据 |

## HTTP 接口

- `GET /api/suggest?q=关键词`：返回 `{"suggestions":["..."]}`。
- `GET /api/search?q=关键词`：返回
  `{"results":[{"id":1,"title":"...","link":"...","abstract":"..."}]}`。

查询会先去除首尾空白，不能为空，且最多包含 200 个 Unicode code point。
API 响应均带有 `Cache-Control: no-store`。错误统一返回
`{"error":"中文可读消息"}`。

## TLV 与 JSON 契约

TLV 帧由 `1 字节 type + 4 字节大端无符号 value 长度 + UTF-8 value`
组成。推荐请求使用 `type=1`，搜索请求使用 `type=2`；请求 value 是处理后的
查询文本，响应 type 必须与请求一致。

搜索服务必须在一条连接中返回且只返回一个完整 TLV 帧。桥接层按帧内声明的长度
读取，收到完整帧后立即处理并主动断开，不要求搜索服务先关闭连接。总等待时间为
5 秒，整帧最大为 2 MiB，并拒绝错误类型、不完整帧、同一批数据中的多余内容、
非法 UTF-8 或非法 JSON。

- `type=1` 的响应 value 必须是 JSON 字符串数组，例如
  `["搜索建议","另一个建议"]`。
- `type=2` 的响应 value 必须是 JSON 对象数组。每项的 `id` 必须是数字或
  字符串，`title`、`link` 和 `abstract` 必须是字符串。

主题按钮使用 Feather Icons 的 `moon`、`sun` 和 `x` 图标（MIT License），图标已
内嵌，无需联网加载资源。
