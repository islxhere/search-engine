#pragma once
#include <cppjieba/Jieba.hpp>

class KeywordProcessor {
public:
    KeywordProcessor();

    void process(const std::string &chDir, const std::string &enDir);

    void create_cn_dict(const std::string &dir, const std::string &outfile);

    void create_cn_index(const std::string &dict, const std::string &index);

    void create_en_dict(const std::string &dir, const std::string &outfile);

    void create_en_index(const std::string &dict, const std::string &index);

private:
    cppjieba::Jieba tokenizer_;
    std::set<std::string> enStopWords_;
    std::set<std::string> chStopWords_;
};
