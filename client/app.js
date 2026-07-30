const elements = {
  form: document.querySelector(".search-form"),
  input: document.querySelector("#search-input"),
  clear: document.querySelector(".clear-button"),
  suggestions: document.querySelector("#suggestions"),
  theme: document.querySelector(".theme-button"),
  welcome: document.querySelector(".welcome"),
  summary: document.querySelector(".results-summary"),
  meta: document.querySelector("#results-meta"),
  resultsLayout: document.querySelector(".results-layout"),
  results: document.querySelector("#results"),
  related: document.querySelector(".related"),
  relatedList: document.querySelector("#related-list"),
  loading: document.querySelector(".loading-state"),
  empty: document.querySelector(".empty-state"),
  error: document.querySelector(".error-state"),
  errorMessage: document.querySelector("#error-message"),
  retry: document.querySelector("#retry-button"),
  liveStatus: document.querySelector("#live-status"),
  suggestionIcon: document.querySelector("#suggestion-icon-template"),
};

const themeMedia = matchMedia("(prefers-color-scheme: dark)");
let suggestions = [];
let activeSuggestion = -1;
let suggestTimer;
let suggestRequest;
let searchRequest;
let lastQuery = "";

function setTheme(theme, persist = true) {
  document.documentElement.dataset.theme = theme;
  document.documentElement.style.colorScheme = theme;
  elements.theme.setAttribute(
    "aria-label",
    theme === "dark" ? "切换到浅色模式" : "切换到深色模式",
  );

  if (persist) {
    try {
      localStorage.setItem("xunji-theme", theme);
    } catch {}
  }
}

function storedTheme() {
  try {
    const theme = localStorage.getItem("xunji-theme");
    return theme === "light" || theme === "dark" ? theme : null;
  } catch {
    return null;
  }
}

function setView(view) {
  const sections = {
    welcome: elements.welcome,
    loading: elements.loading,
    empty: elements.empty,
    error: elements.error,
    results: elements.resultsLayout,
  };

  for (const [name, section] of Object.entries(sections)) {
    section.hidden = name !== view;
  }
  elements.summary.hidden = !["loading", "empty", "error", "results"].includes(view);
}

function announce(message) {
  elements.liveStatus.textContent = "";
  requestAnimationFrame(() => {
    elements.liveStatus.textContent = message;
  });
}

function queryValue() {
  return elements.input.value.trim();
}

function syncClearButton() {
  elements.clear.hidden = elements.input.value.length === 0;
}

function closeSuggestions() {
  suggestions = [];
  activeSuggestion = -1;
  elements.suggestions.replaceChildren();
  elements.suggestions.hidden = true;
  elements.input.setAttribute("aria-expanded", "false");
  elements.input.removeAttribute("aria-activedescendant");
}

function setActiveSuggestion(index) {
  const options = [...elements.suggestions.children];
  if (!options.length) return;

  activeSuggestion = (index + options.length) % options.length;
  options.forEach((option, optionIndex) => {
    option.setAttribute("aria-selected", String(optionIndex === activeSuggestion));
  });

  const active = options[activeSuggestion];
  elements.input.setAttribute("aria-activedescendant", active.id);
  active.scrollIntoView({ block: "nearest" });
}

function selectSuggestion(value, searchNow = false) {
  elements.input.value = value;
  syncClearButton();
  closeSuggestions();
  if (searchNow) search(value);
}

function renderSuggestions(items) {
  suggestions = items.filter((item, index) => item && items.indexOf(item) === index).slice(0, 8);
  activeSuggestion = -1;
  elements.suggestions.replaceChildren();

  if (!suggestions.length || document.activeElement !== elements.input) {
    closeSuggestions();
    return;
  }

  suggestions.forEach((suggestion, index) => {
    const option = document.createElement("li");
    option.id = `suggestion-${index}`;
    option.role = "option";
    option.setAttribute("aria-selected", "false");
    const label = document.createElement("span");
    label.textContent = suggestion;
    option.append(elements.suggestionIcon.content.cloneNode(true), label);
    option.addEventListener("pointerdown", (event) => {
      event.preventDefault();
    });
    option.addEventListener("click", () => {
      selectSuggestion(suggestion, true);
    });
    elements.suggestions.append(option);
  });

  elements.suggestions.hidden = false;
  elements.input.setAttribute("aria-expanded", "true");
}

async function api(path, signal) {
  const response = await fetch(path, { signal, headers: { Accept: "application/json" } });
  const payload = await response.json().catch(() => ({}));
  if (!response.ok) {
    throw new Error(payload.error || "搜索服务返回了无法识别的响应。");
  }
  return payload;
}

async function loadSuggestions(query, renderList = true) {
  suggestRequest?.abort();
  suggestRequest = new AbortController();

  try {
    const payload = await api(
      `/api/suggest?q=${encodeURIComponent(query)}`,
      suggestRequest.signal,
    );
    if (query !== queryValue()) return [];
    if (renderList) renderSuggestions(payload.suggestions);
    return payload.suggestions;
  } catch (error) {
    if (error.name !== "AbortError" && renderList) closeSuggestions();
    return [];
  }
}

function safeHttpUrl(value) {
  try {
    const url = new URL(value);
    return ["http:", "https:"].includes(url.protocol) ? url.href : null;
  } catch {
    return null;
  }
}

function renderResults(items) {
  elements.results.replaceChildren();

  for (const item of items) {
    const result = document.createElement("li");
    result.className = "result";

    const heading = document.createElement("h2");
    const href = safeHttpUrl(item.link);
    if (href) {
      const link = document.createElement("a");
      link.href = href;
      link.target = "_blank";
      link.rel = "noopener noreferrer";
      link.textContent = item.title;
      heading.append(link);
    } else {
      heading.textContent = item.title;
    }

    const source = document.createElement("p");
    source.className = "result-source";
    source.textContent = href || "来源地址不可用";

    const summary = document.createElement("p");
    summary.className = "result-abstract";
    summary.textContent = item.abstract;

    result.append(heading, source, summary);
    elements.results.append(result);
  }
}

function renderRelated(items) {
  const unique = items.filter((item, index) => item && item !== lastQuery && items.indexOf(item) === index);
  elements.relatedList.replaceChildren();

  for (const item of unique.slice(0, 8)) {
    const listItem = document.createElement("li");
    const button = document.createElement("button");
    button.type = "button";
    button.textContent = item;
    button.addEventListener("click", () => selectSuggestion(item, true));
    listItem.append(button);
    elements.relatedList.append(listItem);
  }

  elements.related.hidden = elements.relatedList.childElementCount === 0;
  elements.resultsLayout.classList.toggle(
    "without-related",
    elements.relatedList.childElementCount === 0,
  );
}

function updateUrl(query, mode) {
  const url = new URL(location.href);
  const method = mode === "replace" || url.searchParams.get("q") === query
    ? "replaceState"
    : "pushState";
  url.searchParams.set("q", query);
  history[method]({}, "", url);
}

async function search(rawQuery, historyMode = "push") {
  const query = rawQuery.trim();
  if (!query) {
    elements.input.focus();
    return;
  }

  lastQuery = query;
  elements.input.value = query;
  syncClearButton();
  clearTimeout(suggestTimer);
  suggestRequest?.abort();
  closeSuggestions();
  searchRequest?.abort();
  const request = new AbortController();
  searchRequest = request;

  if (historyMode) updateUrl(query, historyMode);

  elements.meta.textContent = "正在检索…";
  setView("loading");
  announce(`正在搜索${query}`);
  const startedAt = performance.now();

  try {
    const searchPayload = await api(
      `/api/search?q=${encodeURIComponent(query)}`,
      request.signal,
    );
    if (request !== searchRequest) return;

    const elapsed = ((performance.now() - startedAt) / 1000).toFixed(2);
    const count = searchPayload.results.length;
    elements.meta.textContent = `找到 ${count} 条结果（用时 ${elapsed} 秒）`;

    if (!count) {
      setView("empty");
      announce(`没有找到与${query}相关的结果`);
      return;
    }

    renderResults(searchPayload.results);
    elements.relatedList.replaceChildren();
    elements.related.hidden = true;
    elements.resultsLayout.classList.remove("without-related");
    setView("results");
    announce(`已找到 ${count} 条相关结果`);

    loadSuggestions(query, false).then((relatedItems) => {
      if (request !== searchRequest) return;
      renderRelated(relatedItems);
    });
  } catch (error) {
    if (request !== searchRequest || error.name === "AbortError") return;
    elements.meta.textContent = "搜索未完成";
    elements.errorMessage.textContent = error.message;
    setView("error");
    announce(`搜索失败：${error.message}`);
  }
}

elements.form.addEventListener("submit", (event) => {
  event.preventDefault();
  if (activeSuggestion >= 0) {
    selectSuggestion(suggestions[activeSuggestion], true);
    return;
  }
  search(queryValue());
});

elements.input.addEventListener("input", () => {
  syncClearButton();
  clearTimeout(suggestTimer);
  const query = queryValue();
  if (!query) {
    suggestRequest?.abort();
    closeSuggestions();
    return;
  }

  suggestTimer = setTimeout(() => loadSuggestions(query), 180);
});

elements.input.addEventListener("focus", () => {
  const query = queryValue();
  if (query) loadSuggestions(query);
});

elements.input.addEventListener("keydown", (event) => {
  if (elements.suggestions.hidden) return;
  if (event.key === "ArrowDown") {
    event.preventDefault();
    setActiveSuggestion(activeSuggestion + 1);
  } else if (event.key === "ArrowUp") {
    event.preventDefault();
    setActiveSuggestion(activeSuggestion - 1);
  } else if (event.key === "Escape") {
    closeSuggestions();
  }
});

elements.input.addEventListener("blur", () => {
  setTimeout(closeSuggestions, 100);
});

elements.clear.addEventListener("click", () => {
  elements.input.value = "";
  syncClearButton();
  closeSuggestions();
  elements.input.focus();
});

elements.theme.addEventListener("click", () => {
  setTheme(document.documentElement.dataset.theme === "dark" ? "light" : "dark");
});

themeMedia.addEventListener("change", ({ matches }) => {
  if (!storedTheme()) setTheme(matches ? "dark" : "light", false);
});

elements.retry.addEventListener("click", () => search(lastQuery, ""));

addEventListener("popstate", () => {
  const query = new URL(location.href).searchParams.get("q")?.trim() || "";
  if (query) {
    search(query, "");
  } else {
    searchRequest?.abort();
    elements.input.value = "";
    syncClearButton();
    closeSuggestions();
    setView("welcome");
  }
});

setTheme(document.documentElement.dataset.theme, false);
syncClearButton();

const initialQuery = new URL(location.href).searchParams.get("q")?.trim();
if (initialQuery) {
  search(initialQuery, "replace");
} else {
  setView("welcome");
  elements.input.focus();
}
