#include "webpage/WebSearcher.h"
#include "webpage/InvertedIndex.h"
#include "webpage/DocLibrary.h"
#include "webpage/TfIdf.h"
#include "core/EnvLoader.h"
#include "core/Utf8Utils.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string_view>
#include <tinyxml2.h>
#include <utility>


namespace {
    using namespace tinyxml2;
    constexpr std::size_t abstract_chars = 50;
    constexpr std::size_t context_chars = 10;

    struct DocFields {
        int id;
        std::string title;
        std::string link;
        std::string content;
    };

    struct CharRange {
        std::size_t begin;
        std::size_t end;
    };

    /**
     * 字符序号：0   1   2   3   4 ...
     * 字符内容：人  工   智  能  正 ...
     * 字节位置：0   3   6   9  12 ...
     *
     * 人[0, 3)
     * 工[3, 6)
     * 智[6, 9)
     * ...
     */
    // 对传入text计算每个utf8字符的偏移位置
    std::vector<std::size_t> utf8_offsets(const std::string_view text) {
        std::vector<std::size_t> offsets;
        auto it = text.begin();
        while (it != text.end()) {
            offsets.push_back(static_cast<std::size_t>(it - text.begin()));
            utf8::next(it, text.end());
        }
        offsets.push_back(text.size());
        return offsets;
    }

    // 静态摘要，返回content的前50个utf8字符
    std::string static_abstract(const std::string_view text, const std::size_t count) {
        const auto offsets = utf8_offsets(text);
        const auto chars = offsets.size() - 1;
        return std::string{text.substr(0, offsets[std::min(count, chars)])};
    }

    DocFields parse_doc(const std::string_view doc) {
        XMLDocument document;
        XMLError eResult = document.Parse(doc.data(), doc.size());
        if (eResult != XML_SUCCESS) throw std::runtime_error("文档数据解析失败");

        const auto *root = document.FirstChildElement("doc");
        if (!root) throw std::runtime_error("缺少 doc 节点");

        int id;
        const auto *id_element = root->FirstChildElement("id");
        if (!id_element || id_element->QueryIntText(&id) != XML_SUCCESS)
            throw std::runtime_error("文档数据缺少有效 id");

        const auto text = [root](const char *name) {
            const auto *element = root->FirstChildElement(name);
            return element != nullptr && element->GetText() != nullptr
                       ? std::string{element->GetText()}
                       : std::string{};
        };

        return {.id = id, .title = text("title"), .link = text("link"), .content = text("content")};
    }

    std::string make_abstract(const std::string &content, const std::set<std::string> &key_words) {
        const auto offsets = utf8_offsets(content);
        const auto chars = offsets.size() - 1;
        std::vector<CharRange> ranges;

        for (const auto &word: key_words) {
            const auto byte_begin = content.find(word);
            if (byte_begin == std::string::npos) continue;

            const auto byte_end = byte_begin + word.size();
            const auto char_begin = static_cast<std::size_t>(
                std::lower_bound(offsets.begin(), offsets.end(), byte_begin) - offsets.begin()
            );
            const auto char_end = static_cast<std::size_t>(
                std::lower_bound(offsets.begin(), offsets.end(), byte_end) - offsets.begin()
            );
            ranges.push_back({
                .begin = char_begin > context_chars ? char_begin - context_chars : 0,
                .end = std::min(chars, char_end + context_chars)
            });
        }

        // 没有在正文中找到查询词时，回退为前 50 个字符的静态摘要。
        if (ranges.empty()) return static_abstract(content, abstract_chars);

        std::sort(ranges.begin(), ranges.end(), [](const CharRange &lhs, const CharRange &rhs) {
            return lhs.begin < rhs.begin;
        });

        // 合并重叠的关键词上下文，避免同一段正文重复出现在摘要中。
        std::vector<CharRange> merged;
        for (const auto &range: ranges) {
            if (merged.empty() || range.begin > merged.back().end) merged.push_back(range);
            else merged.back().end = std::max(merged.back().end, range.end);
        }

        std::string result;
        std::size_t result_chars = 0;
        for (const auto &[begin, end]: merged) {
            if (!result.empty()) {
                if (result_chars == abstract_chars) break;
                result += "…";
                ++result_chars;
            }

            const auto count = std::min(end - begin, abstract_chars - result_chars);
            result.append(
                content,
                offsets[begin],
                offsets[begin + count] - offsets[begin]
            );
            result_chars += count;
            if (count < end - begin) break;
        }
        return result;
    }
}


WebSearcher::WebSearcher(const DocLibrary &docs, const InvertedIndex &index)
    : docs_(docs), index_(index) {
    const auto stopwords = EnvLoader::se_stopwords_dir();
    std::ifstream ch_file{stopwords + "/cn_stopwords.txt"};
    std::ifstream en_file{stopwords + "/en_stopwords.txt"};
    if (!ch_file || !en_file)
        throw std::runtime_error("无法加载停用词库: " + stopwords);

    std::string stopword;
    while (ch_file >> stopword) stop_words_.insert(stopword);
    while (en_file >> stopword) stop_words_.insert(stopword);
}

std::vector<int> WebSearcher::rank_docs(const std::string &query) {
    std::map<std::string, int> key_words;
    std::vector<std::string> raw_words;

    // 查询和离线建库使用相同的分词、中文过滤及停用词规则。
    tokenizer_.Cut(query, raw_words);
    int total_word = 0;
    for (const auto &word: raw_words) {
        if (!utf8_utils::is_all_chinese(word)) continue;
        if (stop_words_.count(word)) continue;
        total_word++;
        key_words[word]++;
    }

    if (total_word == 0 || docs_.size() == 0) return {};

    std::vector<const std::vector<InvertedIndex::Posting> *> posting_lists;
    std::vector<double> query_weights;
    posting_lists.reserve(key_words.size());
    query_weights.reserve(key_words.size());

    // postings 数量就是该词的文档频率，用于计算查询词的 TF-IDF。
    double query_norm_sq = 0;
    for (const auto &[word, freq]: key_words) {
        const auto &postings = index_.get_postings(word);
        if (postings.empty()) return {};

        posting_lists.push_back(&postings);
        const double weight = tfidf::weight(
            freq, total_word, docs_.size(), postings.size()
        );
        query_weights.push_back(weight);
        query_norm_sq += weight * weight;
    }

    const double query_norm = std::sqrt(query_norm_sq);
    if (query_norm == 0) return {};
    for (auto &weight: query_weights) weight /= query_norm;

    // 查询向量和文档向量都已归一化，二者模长均为 1：
    // cos(q, d) = q·d / (|q||d|) = q·d，因此直接累加点积即可。
    std::map<int, double> scores;
    for (const auto &[docid, weight]: *posting_lists.front())
        scores.emplace(docid, query_weights.front() * weight);

    // 每轮只保留同时出现在下一个 postings 中的文档，实现 AND 查询。
    for (std::size_t i = 1; i < posting_lists.size() && !scores.empty(); ++i) {
        std::map<int, double> common_scores;
        for (const auto &[docid, weight]: *posting_lists[i]) {
            const auto it = scores.find(docid);
            if (it != scores.end())
                common_scores.emplace(
                    docid,
                    it->second + query_weights[i] * weight
                );
        }
        scores = std::move(common_scores);
    }

    std::vector<std::pair<int, double>> ranked_docs;
    ranked_docs.reserve(scores.size());
    for (const auto &[docid, score]: scores)
        ranked_docs.emplace_back(docid, score);

    // 分数相同时按文档 ID 排序，保证结果稳定。
    std::sort(ranked_docs.begin(), ranked_docs.end(), [](const auto &lhs, const auto &rhs) {
        if (lhs.second != rhs.second) return lhs.second > rhs.second;
        return lhs.first < rhs.first;
    });

    std::vector<int> docids;
    docids.reserve(ranked_docs.size());
    for (const auto &[docid, _]: ranked_docs)
        docids.push_back(docid);
    return docids;
}

std::string WebSearcher::search(const std::string &query) {
    std::vector<std::string> raw_words;
    tokenizer_.Cut(query, raw_words);

    std::set<std::string> key_words;
    for (const auto &word: raw_words)
        if (utf8_utils::is_all_chinese(word) && !stop_words_.count(word))
            key_words.insert(word);

    auto response = nlohmann::json::array();
    for (const auto docid: rank_docs(query)) {
        const auto doc = parse_doc(docs_.get_doc(docid));
        response.push_back({
            {"id", doc.id},
            {"title", doc.title},
            {"link", doc.link},
            {"abstract", make_abstract(doc.content, key_words)}
        });
    }
    return response.dump();
}
