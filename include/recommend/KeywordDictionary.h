#pragma once

#include <cstddef>
#include <string>
#include <vector>


class KeywordDictionary {
public:
    struct Entry {
        std::string word;
        int frequency;
    };

    explicit KeywordDictionary(const std::string &path);

    const Entry &get(std::size_t line) const;

    std::size_t size() const;

private:
    std::vector<Entry> entries_;
};
