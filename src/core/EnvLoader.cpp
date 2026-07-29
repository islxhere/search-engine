#include "core/EnvLoader.h"

#include <cstdlib>
#include <stdexcept>

std::optional<std::string> EnvLoader::optional(const std::string_view name) {
    const std::string key{name};
    const char *value = std::getenv(key.c_str());
    if (value == nullptr || *value == '\0')
        return std::nullopt;
    return value;
}

std::string EnvLoader::required(const std::string_view name) {
    std::optional<std::string> value = optional(name);
    if (!value)
        throw std::runtime_error("缺少必填环境变量: " + std::string(name));
    return *value;
}

std::string EnvLoader::se_data_dict_dir() { return required("SE_DATA_DIR1"); }

std::string EnvLoader::se_corpus_dir() { return required("SE_CORPUS_DIR"); }

std::string EnvLoader::se_stopwords_dir() { return required("SE_STOPWORDS_DIR"); }
