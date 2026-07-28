#pragma once

#include <optional>
#include <string>
#include <string_view>

class EnvLoader {
public:
    EnvLoader() = delete;

    ~EnvLoader() = delete;

    static std::string se_data_dir();

    static std::string se_corpus_dir();

    static std::string se_stopwords_dir();

private:
    static std::string required(std::string_view name);

    static std::optional<std::string> optional(std::string_view name);
};
