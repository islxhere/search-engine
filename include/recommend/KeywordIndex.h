#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>


class KeywordIndex {
public:
    KeywordIndex(const std::string &path, std::size_t dictionary_size);

    const std::vector<std::size_t> &get_lines(const std::string &character) const;

private:
    std::unordered_map<std::string, std::vector<std::size_t> > lines_;
};
