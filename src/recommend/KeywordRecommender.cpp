#include "recommend/KeywordRecommender.h"

#include "core/Utf8Utils.h"
#include "recommend/KeywordDictionary.h"
#include "recommend/KeywordIndex.h"

#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>
#include <queue>
#include <utfcpp/utf8.h>


namespace {
    constexpr std::size_t top_k = 8;

    struct Candidate {
        std::string word;
        int frequency;
    };

    bool is_better(const Candidate &lhs, const Candidate &rhs) {
        if (lhs.frequency != rhs.frequency) return lhs.frequency > rhs.frequency;
        return lhs.word < rhs.word;
    }

    std::vector<char32_t> utf8_characters(const std::string &text) {
        std::vector<char32_t> characters;
        auto current = text.begin();
        while (current != text.end()) characters.push_back(utf8::next(current, text.end()));
        return characters;
    }

    bool is_english(const std::string &text) {
        return !text.empty() && std::all_of(text.begin(), text.end(), [](const unsigned char ch) {
            return std::isalpha(ch) != 0;
        });
    }

    std::string lowercase(std::string text) {
        for (auto &ch: text) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return text;
    }

    bool has_prefix(const std::string &word, const std::string &prefix) {
        return word.size() >= prefix.size() && word.compare(0, prefix.size(), prefix) == 0;
    }

    std::string make_response(std::priority_queue<Candidate, std::vector<Candidate>, decltype(&is_better)> &heap) {
        std::vector<Candidate> ranked;
        ranked.reserve(heap.size());
        while (!heap.empty()) {
            ranked.push_back(heap.top());
            heap.pop();
        }
        std::sort(ranked.begin(), ranked.end(), is_better);

        auto response = nlohmann::json::array();
        for (const auto &candidate: ranked) response.push_back(candidate.word);
        return response.dump();
    }
}


KeywordRecommender::KeywordRecommender(
    const KeywordDictionary &cn_dictionary,
    const KeywordIndex &cn_index,
    const KeywordDictionary &en_dictionary,
    const KeywordIndex &en_index
)
    : cn_dictionary_(cn_dictionary),
      cn_index_(cn_index),
      en_dictionary_(en_dictionary),
      en_index_(en_index) {}

std::string KeywordRecommender::recommend(const std::string &query) const {
    if (query.empty()) return nlohmann::json::array().dump();

    std::priority_queue<Candidate, std::vector<Candidate>, decltype(&is_better)> heap{&is_better};
    const auto consider = [&heap](const KeywordDictionary::Entry &entry) {
        const Candidate candidate{entry.word, entry.frequency};
        if (heap.size() < top_k) heap.push(candidate);
        else if (is_better(candidate, heap.top())) {
            heap.pop();
            heap.push(candidate);
        }
    };

    if (utf8_utils::is_all_chinese(query)) {
        const auto characters = utf8_characters(query);
        std::string key;
        utf8::append(characters.front(), std::back_inserter(key));
        for (const auto line: cn_index_.get_lines(key)) {
            const auto &entry = cn_dictionary_.get(line);
            if (has_prefix(entry.word, query)) consider(entry);
        }
    } else if (is_english(query)) {
        const auto normalized = lowercase(query);
        for (const auto line: en_index_.get_lines(std::string(1, normalized.front()))) {
            const auto &entry = en_dictionary_.get(line);
            if (has_prefix(entry.word, normalized)) consider(entry);
        }
    }

    return make_response(heap);
}
