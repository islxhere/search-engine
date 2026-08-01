#include "recommend/KeywordIndex.h"

#include <fstream>
#include <sstream>
#include <stdexcept>


KeywordIndex::KeywordIndex(const std::string &path, const std::size_t dictionary_size) {
    std::ifstream input{path};
    if (!input) throw std::runtime_error("无法加载索引库: " + path);

    std::string line;
    while (getline(input, line)) {
        std::istringstream stream{line};
        std::string character;
        if (!(stream >> character)) throw std::runtime_error("索引记录格式错误: " + path);

        std::vector<std::size_t> lines;
        std::size_t line_number;
        while (stream >> line_number) {
            if (line_number == 0 || line_number > dictionary_size)
                throw std::runtime_error("索引行号越界: " + path);
            lines.push_back(line_number - 1);
        }
        if (lines.empty()) throw std::runtime_error("索引记录缺少行号: " + path);
        lines_.emplace(std::move(character), std::move(lines));
    }
}

const std::vector<std::size_t> &KeywordIndex::get_lines(const std::string &character) const {
    static const std::vector<std::size_t> empty;
    const auto it = lines_.find(character);
    return it == lines_.end() ? empty : it->second;
}
