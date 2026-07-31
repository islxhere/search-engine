#include "recommend/KeywordProcessor.h"
#include "core/DirectoryScanner.h"
#include "core/EnvLoader.h"
#include "core/Utf8Utils.h"

#include <utfcpp/utf8.h>

KeywordProcessor::KeywordProcessor() {
    const auto stopwords = EnvLoader::se_stopwords_dir();
    const std::string ch_dir = stopwords + "/cn_stopwords.txt";
    const std::string en_dir = stopwords + "/en_stopwords.txt";

    std::ifstream chFile{ch_dir};
    std::ifstream enFile{en_dir};

    std::string stopword;

    while (chFile >> stopword) chStopWords_.insert(stopword);
    while (enFile >> stopword) enStopWords_.insert(stopword);
}

void KeywordProcessor::process(const std::string &raw, const std::string &output) {
    build_cn_dict(raw + "/CN", output + "/dict_cn.dat");
    build_cn_index(output + "/dict_cn.dat", output + "/index_cn.dat");
    build_en_dict(raw + "/EN", output + "/dict_en.dat");
    build_en_index(output + "/dict_en.dat", output + "/index_en.dat");
}

void KeywordProcessor::build_cn_dict(const std::string &dir, const std::string &outfile) {
    auto files = DirectoryScanner::scan(dir);
    std::map<std::string, int> wordCount;

    for (auto &file: files) {
        std::ifstream ifs{file};
        std::string line;
        while (getline(ifs, line)) {
            std::vector<std::string> words;
            tokenizer_.Cut(line, words);
            for (auto &word: words) {
                if (!utf8_utils::is_all_chinese(word)) continue;
                if (chStopWords_.count(word)) continue;
                wordCount[word]++;
            }
        }
    }

    std::ofstream ofs{outfile};
    using Pair = std::pair<std::string, int>;
    std::vector<Pair> vec{wordCount.begin(), wordCount.end()};
    std::sort(vec.begin(), vec.end(), [](const Pair &a, const Pair &b) -> bool { return a.second > b.second; });
    for (auto &[word, count]: vec)
        ofs << word << " " << count << std::endl;
    std::cout << "[√] 中文词典库生成" << std::endl;
}

void KeywordProcessor::build_cn_index(const std::string &dict, const std::string &index) {
    std::ifstream ifs{dict};
    std::string line;
    std::map<std::string, std::set<int> > charIndex;
    int lineNo = 0;

    while (getline(ifs, line)) {
        ++lineNo;
        std::istringstream iss{line};
        std::string word;
        int freq;
        iss >> word >> freq;

        const char *curr = word.c_str();
        const char *end = word.c_str() + word.size();

        while (curr != end) {
            auto start = curr;
            utf8::next(curr, end);
            std::string character = std::string{start, curr};
            charIndex[character].insert(lineNo);
        }
    }

    std::ofstream ofs{index};
    for (auto &[ch, indexes]: charIndex) {
        ofs << ch;
        for (int n: indexes) ofs << " " << n;
        ofs << std::endl;
    }
    std::cout << "[√] 中文索引库生成" << std::endl;
}

void KeywordProcessor::build_en_dict(const std::string &dir, const std::string &outfile) {
    auto files = DirectoryScanner::scan(dir);
    std::map<std::string, int> wordCount;

    for (auto &file: files) {
        std::ifstream ifs{file};
        std::string line;
        while (getline(ifs, line)) {
            for (auto &ch: line) {
                if (isalpha(ch)) ch = static_cast<char>(tolower(ch));
                else ch = ' ';
            }
            std::istringstream iss{line};
            std::string word;
            while (iss >> word) {
                if (enStopWords_.count(word)) continue;
                wordCount[word]++;
            }
        }
    }

    std::ofstream ofs{outfile};
    using Pair = std::pair<std::string, int>;
    std::vector<Pair> vec{wordCount.begin(), wordCount.end()};
    std::sort(vec.begin(), vec.end(), [](const Pair &a, const Pair &b) -> bool { return a.second > b.second; });
    for (auto &[word, count]: vec)
        ofs << word << " " << count << std::endl;

    std::cout << "[√] 英文词典库生成" << std::endl;
}

void KeywordProcessor::build_en_index(const std::string &dict, const std::string &index) {
    std::ifstream ifs{dict};
    std::string line;
    std::map<char, std::set<int> > charIndex;
    int lineNo = 0;

    while (getline(ifs, line)) {
        ++lineNo;
        std::istringstream iss{line};
        std::string word;
        int _;
        iss >> word >> _;

        for (auto &ch: word) charIndex[ch].insert(lineNo);
    }

    std::ofstream ofs{index};
    for (auto &[ch, indexes]: charIndex) {
        ofs << ch;
        for (int n: indexes) ofs << " " << n;
        ofs << std::endl;
    }

    std::cout << "[√] 英文索引库生成" << std::endl;
}
