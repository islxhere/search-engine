import fs from 'node:fs/promises';
import http from 'node:http';
import net from 'node:net';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const CLIENT_HOST = process.env.CLIENT_HOST || '127.0.0.1';
const CLIENT_PORT = readPort('CLIENT_PORT', 4173);
const SEARCH_SERVER_HOST = process.env.SEARCH_SERVER_HOST || '127.0.0.1';
const SEARCH_SERVER_PORT = readPort('SEARCH_SERVER_PORT', 8888);
const MOCK_DATA = process.env.MOCK_DATA === '1';
const TIMEOUT_MS = 5_000;
const MAX_RESPONSE_BYTES = 2 * 1024 * 1024;
const CLIENT_DIR = path.dirname(fileURLToPath(import.meta.url));

const STATIC_FILES = new Map([
  ['/', ['index.html', 'text/html; charset=utf-8']],
  ['/index.html', ['index.html', 'text/html; charset=utf-8']],
  ['/styles.css', ['styles.css', 'text/css; charset=utf-8']],
  ['/app.js', ['app.js', 'text/javascript; charset=utf-8']],
]);

const API_ROUTES = new Map([
  ['/api/suggest', 1],
  ['/api/search', 2],
]);

class BridgeError extends Error {
  constructor(statusCode, message) {
    super(message);
    this.statusCode = statusCode;
  }
}

function readPort(name, fallback) {
  const raw = process.env[name];
  if (raw === undefined || raw === '') return fallback;

  const port = Number(raw);
  if (!Number.isInteger(port) || port < 1 || port > 65_535) {
    throw new Error(`${name} 必须是 1 到 65535 的整数`);
  }
  return port;
}

function sendJson(response, statusCode, payload, noStore = false, extraHeaders = {}) {
  if (response.destroyed) return;

  const body = Buffer.from(JSON.stringify(payload));
  response.writeHead(statusCode, {
    'Content-Type': 'application/json; charset=utf-8',
    'Content-Length': body.length,
    ...(noStore ? { 'Cache-Control': 'no-store' } : {}),
    ...extraHeaders,
  });
  response.end(body);
}

function decodeResponse(frame, expectedType) {
  if (frame.length < 5) {
    throw new BridgeError(502, '搜索服务返回了不完整的数据');
  }

  const type = frame.readUInt8(0);
  const valueLength = frame.readUInt32BE(1);
  if (type !== expectedType) {
    throw new BridgeError(502, '搜索服务返回了错误的响应类型');
  }
  if (valueLength + 5 > MAX_RESPONSE_BYTES) {
    throw new BridgeError(502, '搜索服务响应过大');
  }
  if (frame.length !== valueLength + 5) {
    throw new BridgeError(502, '搜索服务返回了不完整或多余的数据');
  }

  try {
    const json = new TextDecoder('utf-8', { fatal: true }).decode(frame.subarray(5));
    return JSON.parse(json);
  } catch {
    throw new BridgeError(502, '搜索服务返回了无效的 JSON');
  }
}

function requestSearch(type, query, httpResponse) {
  const value = Buffer.from(query, 'utf8');
  const requestFrame = Buffer.allocUnsafe(value.length + 5);
  requestFrame.writeUInt8(type, 0);
  requestFrame.writeUInt32BE(value.length, 1);
  value.copy(requestFrame, 5);

  return new Promise((resolve, reject) => {
    let responseBuffer = Buffer.alloc(0);
    let settled = false;
    let timer;

    const socket = net.createConnection({
      host: SEARCH_SERVER_HOST,
      port: SEARCH_SERVER_PORT,
    });

    const finish = (error, result) => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      httpResponse.off('close', onClientClose);
      socket.destroy();
      if (error) {
        reject(error);
      } else {
        resolve(result);
      }
    };

    const onClientClose = () => {
      if (!httpResponse.writableEnded) {
        finish(new BridgeError(502, '客户端已断开连接'));
      }
    };

    httpResponse.once('close', onClientClose);
    timer = setTimeout(
      () => finish(new BridgeError(504, '搜索服务响应超时')),
      TIMEOUT_MS,
    );

    socket.once('connect', () => {
      if (!settled) socket.write(requestFrame);
    });
    socket.on('data', (chunk) => {
      if (settled) return;
      const size = responseBuffer.length + chunk.length;
      if (size > MAX_RESPONSE_BYTES) {
        finish(new BridgeError(502, '搜索服务响应过大'));
        return;
      }

      responseBuffer = Buffer.concat([responseBuffer, chunk], size);
      if (responseBuffer.length < 5) return;

      const frameLength = responseBuffer.readUInt32BE(1) + 5;
      if (frameLength > MAX_RESPONSE_BYTES) {
        finish(new BridgeError(502, '搜索服务响应过大'));
        return;
      }
      if (responseBuffer.length < frameLength) return;
      if (responseBuffer.length > frameLength) {
        finish(new BridgeError(502, '搜索服务返回了多余数据'));
        return;
      }

      try {
        finish(null, decodeResponse(responseBuffer, type));
      } catch (error) {
        finish(error);
      }
    });
    socket.once('end', () => {
      if (!settled) {
        finish(new BridgeError(502, '搜索服务提前断开了连接'));
      }
    });
    socket.once('error', () => {
      finish(new BridgeError(502, '无法连接搜索服务'));
    });
    socket.once('close', () => {
      if (!settled) {
        finish(new BridgeError(502, '搜索服务提前断开了连接'));
      }
    });
  });
}

function formatApiResponse(type, payload) {
  if (type === 1) {
    if (!Array.isArray(payload) || !payload.every((item) => typeof item === 'string')) {
      throw new BridgeError(502, '搜索服务返回了无效的推荐结果');
    }
    return { suggestions: payload };
  }

  const valid = Array.isArray(payload) && payload.every(
    (item) => item !== null
      && typeof item === 'object'
      && !Array.isArray(item)
      && (typeof item.id === 'number' || typeof item.id === 'string')
      && typeof item.title === 'string'
      && typeof item.link === 'string'
      && typeof item.abstract === 'string',
  );
  if (!valid) {
    throw new BridgeError(502, '搜索服务返回了无效的搜索结果');
  }
  return { results: payload };
}

function mockPayload(type, query) {
  if (type === 1) {
    return [
      `${query} 是什么`,
      `${query} 最新进展`,
      `${query} 应用案例`,
      `${query} 开源项目`,
      `${query} 入门指南`,
    ];
  }

  const slug = encodeURIComponent(query);
  return [
    {
      id: 1,
      title: `${query}：从概念到实践的入门指南`,
      link: `https://example.com/guides/${slug}`,
      abstract: `用一篇文章了解${query}的核心概念、常见术语与开始探索的方法。`,
    },
    {
      id: 2,
      title: `如何用${query}解决真实问题`,
      link: `https://example.com/stories/${slug}`,
      abstract: `整理几个来自产品、研究与日常工作的案例，看看${query}能带来什么。`,
    },
    {
      id: 3,
      title: `${query}资源清单`,
      link: `https://example.com/resources/${slug}`,
      abstract: `适合继续阅读的工具、社区与公开资料，按上手难度做了简要分类。`,
    },
    {
      id: 4,
      title: `${query}的下一步：趋势与思考`,
      link: `https://example.com/insights/${slug}`,
      abstract: `从近期变化出发，梳理值得关注的方向，以及实践中需要留意的问题。`,
    },
  ];
}

async function handleApi(response, url, type) {
  const query = (url.searchParams.get('q') || '').trim();
  if (!query) {
    sendJson(response, 400, { error: '查询内容不能为空' }, true);
    return;
  }
  if (Array.from(query).length > 200) {
    sendJson(response, 400, { error: '查询内容不能超过 200 个字符' }, true);
    return;
  }

  try {
    const payload = MOCK_DATA
      ? mockPayload(type, query)
      : await requestSearch(type, query, response);
    sendJson(response, 200, formatApiResponse(type, payload), true);
  } catch (error) {
    if (response.destroyed) return;
    if (error instanceof BridgeError) {
      sendJson(response, error.statusCode, { error: error.message }, true);
    } else {
      sendJson(response, 500, { error: '服务内部错误' }, true);
    }
  }
}

async function handleRequest(request, response) {
  let url;
  try {
    url = new URL(request.url, 'http://localhost');
  } catch {
    sendJson(response, 400, { error: '请求地址无效' });
    return;
  }

  const isApi = url.pathname.startsWith('/api/');
  if (request.method !== 'GET') {
    sendJson(response, 405, { error: '仅支持 GET 请求' }, isApi, { Allow: 'GET' });
    return;
  }

  const type = API_ROUTES.get(url.pathname);
  if (type !== undefined) {
    await handleApi(response, url, type);
    return;
  }

  const staticFile = STATIC_FILES.get(url.pathname);
  if (!staticFile) {
    sendJson(response, 404, { error: '请求的资源不存在' }, isApi);
    return;
  }

  try {
    const [filename, contentType] = staticFile;
    const body = await fs.readFile(path.join(CLIENT_DIR, filename));
    if (response.destroyed) return;
    response.writeHead(200, {
      'Content-Type': contentType,
      'Content-Length': body.length,
      'X-Content-Type-Options': 'nosniff',
    });
    response.end(body);
  } catch (error) {
    const missing = error && error.code === 'ENOENT';
    sendJson(
      response,
      missing ? 404 : 500,
      { error: missing ? '请求的资源不存在' : '读取网页资源失败' },
    );
  }
}

const server = http.createServer((request, response) => {
  handleRequest(request, response).catch(() => {
    sendJson(response, 500, { error: '服务内部错误' });
  });
});

server.listen(CLIENT_PORT, CLIENT_HOST, () => {
  const mode = MOCK_DATA ? '（演示数据模式）' : '';
  console.log(`搜索客户端已启动：http://${CLIENT_HOST}:${CLIENT_PORT}${mode}`);
});
