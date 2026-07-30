#pragma once
#include <cppjieba/Jieba.hpp>
#include <set>
#include <string>

class KeywordProcessor {
public:
    KeywordProcessor();

    void process(const std::string &raw, const std::string &output);

private:
    void build_cn_dict(const std::string &dir, const std::string &outfile);

    void build_cn_index(const std::string &dict, const std::string &index);

    void build_en_dict(const std::string &dir, const std::string &outfile);

    void build_en_index(const std::string &dict, const std::string &index);

    cppjieba::Jieba tokenizer_;
    std::set<std::string> enStopWords_;
    std::set<std::string> chStopWords_;
};
