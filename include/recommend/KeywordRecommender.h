#pragma once

#include <string>

class KeywordDictionary;
class KeywordIndex;

class KeywordRecommender {
public:
    KeywordRecommender(
        const KeywordDictionary &cn_dictionary,
        const KeywordIndex &cn_index,
        const KeywordDictionary &en_dictionary,
        const KeywordIndex &en_index
    );

    std::string recommend(const std::string &query) const;

private:
    const KeywordDictionary &cn_dictionary_;
    const KeywordIndex &cn_index_;
    const KeywordDictionary &en_dictionary_;
    const KeywordIndex &en_index_;
};
