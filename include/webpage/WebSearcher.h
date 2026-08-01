#pragma once

#include <set>
#include <string>
#include <vector>
#include <cppjieba/Jieba.hpp>


class InvertedIndex;
class DocLibrary;

class WebSearcher {
public:
    WebSearcher(const DocLibrary &docs, const InvertedIndex &index);

    std::string search(const std::string &query); // 对外搜索并返回 JSON

private:
    std::vector<int> rank_docs(const std::string &query); // 内部检索、打分、排序

    const DocLibrary &docs_;
    const InvertedIndex &index_;
    cppjieba::Jieba tokenizer_;
    std::set<std::string> stop_words_;
};
