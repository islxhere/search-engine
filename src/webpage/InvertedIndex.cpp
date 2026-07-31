#include "webpage/InvertedIndex.h"

#include <fstream>
#include <sstream>

InvertedIndex::InvertedIndex(const std::string &index_path) {
    std::ifstream ifs{index_path};
    std::string record;
    while (getline(ifs, record)) {
        std::istringstream iss{record};
        std::string word;
        iss >> word;

        std::vector<Posting> postings;
        Posting posting{};
        while (iss >> posting.docid >> posting.weight) { postings.push_back(posting); }

        index_.emplace(std::move(word), std::move(postings));
    }
}

const std::vector<InvertedIndex::Posting> &InvertedIndex::get_postings(const std::string &word) const {
    // 未命中的关键词共享同一个只读空表，避免每次查询都复制 postings。
    static const std::vector<Posting> empty;
    auto it = index_.find(word);
    return it == index_.end() ? empty : it->second;
}
