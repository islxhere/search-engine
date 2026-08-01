#include "recommend/KeywordDictionary.h"

#include <fstream>
#include <sstream>
#include <stdexcept>


KeywordDictionary::KeywordDictionary(const std::string &path) {
    std::ifstream input{path};
    if (!input) throw std::runtime_error("无法加载词典库: " + path);

    std::string line;
    while (getline(input, line)) {
        std::istringstream stream{line};
        Entry entry;
        if (!(stream >> entry.word >> entry.frequency) || entry.frequency < 0)
            throw std::runtime_error("词典记录格式错误: " + path);
        entries_.push_back(std::move(entry));
    }
}

const KeywordDictionary::Entry &KeywordDictionary::get(const std::size_t line) const { return entries_.at(line); }

std::size_t KeywordDictionary::size() const { return entries_.size(); }
