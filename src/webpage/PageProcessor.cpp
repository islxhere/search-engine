#include "webpage/PageProcessor.h"
#include "core/DirectoryScanner.h"
#include "core/EnvLoader.h"

#include <iostream>
#include <tinyxml2.h>
#include <regex>
#include <utfcpp/utf8.h>


using std::regex;
using std::regex_constants::icase;
using namespace tinyxml2;

static std::string removeHtmlTags(const std::string &html) {
    std::string text = html;

    // 1. 去掉 script 标签及其中内容
    text = regex_replace(
        text,
        regex(R"(<script[\s\S]*?</script>)", icase),
        ""
    );

    // 2. 去掉 style 标签及其中内容
    text = regex_replace(
        text,
        regex(R"(<style[\s\S]*?</style>)", icase),
        ""
    );

    // 3. 去掉 HTML 注释
    text = regex_replace(
        text,
        regex(R"(<!--[\s\S]*?-->)"),
        ""
    );

    // 4. 把一些块级标签替换成换行，避免文字全部挤在一起
    text = regex_replace(
        text,
        regex(R"(<\s*/?\s*(p|br|div|h[1-6]|li|ul|ol)[^>]*>)", icase),
        "\n"
    );

    // 5. 去掉所有剩余 HTML 标签
    text = regex_replace(
        text,
        regex(R"(<[^>]+>)"),
        ""
    );

    // 6. 处理一些常见 HTML 实体
    text = regex_replace(text, regex(R"(&nbsp;)"), " ");
    text = regex_replace(text, regex(R"(&amp;)"), "&");
    text = regex_replace(text, regex(R"(&lt;)"), "<");
    text = regex_replace(text, regex(R"(&gt;)"), ">");
    text = regex_replace(text, regex(R"(&quot;)"), "\"");
    text = regex_replace(text, regex(R"(&apos;)"), "'");

    // 7. 多个空白压缩成一个空格
    text = regex_replace(text, regex(R"([ \t\r\f\v]+)"), " ");

    // 8. 多个换行压缩
    text = regex_replace(text, regex(R"(\n\s*\n+)"), "\n");

    return text;
}

PageProcessor::PageProcessor() {
    const auto stopwords = EnvLoader::se_stopwords_dir();
    const std::string ch_dir = stopwords + "/cn_stopwords.txt";
    const std::string en_dir = stopwords + "/en_stopwords.txt";

    std::ifstream chFile{ch_dir};
    std::ifstream enFile{en_dir};

    std::string stopword;

    while (chFile >> stopword) stop_words_.insert(stopword);
    while (enFile >> stopword) stop_words_.insert(stopword);
}

void PageProcessor::process(const std::string &raw, const std::string &output) {
    process_docs(raw);
    dedup_docs();
    build_webpage_offset(output + "/pages.dat", output + "/offsets.dat");
    build_inverted_index(output + "/inverted_index.dat");
}

void PageProcessor::process_docs(const std::string &dir) {
    auto files = DirectoryScanner::scan(dir);
    for (auto &file: files) {
        XMLDocument document;
        XMLError eResult = document.LoadFile(file.c_str());

        // Error process
        if (eResult != XML_SUCCESS) {
            std::cerr << "[-] 提取语料库文档: " << document.ErrorStr() << std::endl;
            continue;
        }

        auto *root = document.RootElement(); // root ==> <rss>...</rss>
        auto *channel = root->FirstChildElement("channel"); // channal ==> <channel>...</channel>

        for (auto *item = channel->FirstChildElement("item");
             item != nullptr;
             item = item->NextSiblingElement("item")) {
            Doc_ doc;
            doc.id = static_cast<int>(docs_.size() + 1);

            auto *link = item->FirstChildElement("link");
            if (link) doc.link = link->GetText() ? link->GetText() : "";

            auto *title = item->FirstChildElement("title");
            if (title && title->GetText()) doc.title = removeHtmlTags(title->GetText());

            auto *content = item->FirstChildElement("content");
            auto *desc = item->FirstChildElement("description");
            if (content && content->GetText()) doc.content = removeHtmlTags(content->GetText());
            else if (desc && desc->GetText()) doc.content = removeHtmlTags(desc->GetText());
            else continue;

            docs_.push_back(std::move(doc));
        }
    }
    std::cout << "[√] 提取语料库文档" << std::endl;
}

void PageProcessor::dedup_docs() {
    std::vector<uint64_t> hash_docs;
    std::vector<Doc_> unique_docs;
    for (auto &doc: docs_) {
        int topN = std::max(5, std::min(200, static_cast<int>(doc.content.size()) / 120));
        uint64_t hash_doc;
        hasher_.make(doc.content, topN, hash_doc);

        bool isDuplicate = false;
        for (auto &hash: hash_docs) {
            if (simhash::Simhasher::isEqual(hash, hash_doc)) {
                isDuplicate = true;
                break;
            }
        }
        if (isDuplicate) continue;

        hash_docs.push_back(hash_doc);
        unique_docs.push_back(doc);
    }

    std::cout << "[√] 文档去重: " << docs_.size() << " ==> " << unique_docs.size() << std::endl;
    docs_ = std::move(unique_docs);
}

void PageProcessor::build_webpage_offset(const std::string &pages_dir, const std::string &offsets_dir) {
    std::ofstream pages{pages_dir, std::ios::binary};
    std::ofstream offsets{offsets_dir};
    size_t offset = 0;

    for (const auto &doc: docs_) {
        XMLDocument document;
        XMLElement *docElem = document.NewElement("doc");
        document.InsertEndChild(docElem);

        XMLElement *id = document.NewElement("id");
        id->SetText(doc.id);
        docElem->InsertEndChild(id);

        XMLElement *link = document.NewElement("link");
        link->SetText(doc.link.c_str());
        docElem->InsertEndChild(link);

        XMLElement *title = document.NewElement("title");
        title->SetText(doc.title.c_str());
        docElem->InsertEndChild(title);

        XMLElement *content = document.NewElement("content");
        content->SetText(doc.content.c_str());
        docElem->InsertEndChild(content);

        XMLPrinter printer;
        document.Print(&printer);
        const std::string page{printer.CStr()};

        pages.write(page.data(), static_cast<std::streamsize>(page.size()));
        offsets << doc.id << ' ' << offset << ' ' << page.size() << '\n';
        offset += page.size();
    }

    if (!pages || !offsets) {
        std::cerr << "[×] 网页库生成" << std::endl;
        return;
    }
    std::cout << "[√] 网页库生成" << std::endl;
    std::cout << "[√] 网页偏移库生成" << std::endl;
}

static bool is_chinese(const char32_t cp) { return cp >= 0x4E00 && cp <= 0x9FFF; }

static bool is_all_chinese(std::string &word) {
    // auto it = utf8::iterator<std::string::const_iterator>{word.begin(), word.begin(), word.end()};
    // auto end = utf8::iterator<std::string::const_iterator>{word.end(), word.begin(), word.end()};
    utf8::iterator it{word.begin(), word.begin(), word.end()};
    utf8::iterator end{word.end(), word.begin(), word.end()};
    for (; it != end; ++it) { if (!is_chinese(*it)) return false; }
    return true;
}

void PageProcessor::build_inverted_index(const std::string &index_dir) {
    // term_freq[i] = words_count[i].second / total_words[i] // 词频(TF): 词语在各文档中的频率
    std::vector<std::map<std::string, int> > words_count(docs_.size()); // 词语在各文档中出现的次数
    std::vector total_words(docs_.size(), 0); // 各文档的总词数
    std::map<std::string, int> doc_freq; // 文档频率(DF): 包含该词语的文档个数

    for (int i = 0; i < docs_.size(); ++i) {
        std::vector<std::string> words;
        tokenizer_.Cut(docs_[i].content, words);

        for (auto &word: words) {
            if (!is_all_chinese(word)) continue;
            if (stop_words_.count(word)) continue;
            total_words[i]++;
            words_count[i][word]++;
        }

        for (auto &[word, _]: words_count[i]) doc_freq[word]++;
    }

    // 计算权重
    size_t N = docs_.size();
    for (int i = 0; i < docs_.size(); ++i) {
        double w_sq_sum = 0;
        std::map<std::string, double> weights;
        for (auto &[word, freq]: words_count[i]) {
            double tf = static_cast<double>(freq) / total_words[i];
            double idf = std::log2(N / doc_freq[word] + 1);
            weights[word] = tf * idf;
            w_sq_sum += weights[word] * weights[word];
        }

        // 权重归一化
        double norm = std::sqrt(w_sq_sum);
        for (auto &[word, wight]: weights)
            inverted_index_[word][i] = wight / norm;
    }

    std::ofstream ofs{index_dir};
    for (auto &[word, doc_map]: inverted_index_) {
        ofs << word;
        for (auto &[docid, weight]: doc_map)
            ofs << " " << docid << " " << weight;
        ofs << std::endl;
    }
    std::cout << "[√] 倒排索引库生成" << std::endl;
}
