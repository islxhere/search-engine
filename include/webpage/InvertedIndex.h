#pragma once
#include <string>
#include <unordered_map>
#include <vector>


class InvertedIndex {
public:
    explicit InvertedIndex(const std::string &index_path);

    struct Posting {
        int docid;
        double weight;
    };

    const std::vector<Posting> &get_postings(const std::string &word) const;

private:
    std::unordered_map<std::string, std::vector<Posting> > index_;
};
