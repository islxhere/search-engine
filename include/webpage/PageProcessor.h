#pragma once
#include <string>
#include <vector>
#include <simhash/Simhasher.hpp>


class PageProcessor {
public:
    PageProcessor();

    void process(const std::string &raw, const std::string &output);

private:
    void process_docs(const std::string &dir);

    void dedup_docs();

    void build_webpage_offset(const std::string &pages_dir, const std::string &offsets_dir);

    void build_inverted_index(const std::string &index_dir);


    struct Doc_ {
        int id;
        std::string link;
        std::string title;
        std::string content;
    };

    std::vector<Doc_> docs_;
    simhash::Simhasher hasher_;
    cppjieba::Jieba tokenizer_;
    std::set<std::string> stop_words_;
    std::map<std::string, std::map<int, double> > inverted_index_;
};
