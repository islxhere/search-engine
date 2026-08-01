#pragma once

#include <string>

#include "recommend/KeywordDictionary.h"
#include "recommend/KeywordIndex.h"
#include "recommend/KeywordRecommender.h"
#include "webpage/DocLibrary.h"
#include "webpage/InvertedIndex.h"
#include "webpage/WebSearcher.h"

class SearchEngineBackend {
public:
    SearchEngineBackend();

    std::string recommend(const std::string &) const;
    std::string search(const std::string &);

private:
    DocLibrary docs_;
    InvertedIndex index_;
    WebSearcher searcher_;
    KeywordDictionary cn_dictionary_;
    KeywordIndex cn_index_;
    KeywordDictionary en_dictionary_;
    KeywordIndex en_index_;
    KeywordRecommender recommender_;
};
