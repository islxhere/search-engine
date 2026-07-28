#include "core/KeywordProcessor.h"
#include "core/DirectoryScanner.h"

#include <utfcpp/utf8.h>

KeywordProcessor::KeywordProcessor() = default;

void KeywordProcessor::process(const std::string &chDir, const std::string &enDir) {
    std::ifstream chFile{chDir};
    std::ifstream enFile{enDir};
    std::string stopword;
    while (chFile >> stopword) chStopWords_.insert(stopword);
    while (enFile >> stopword) enStopWords_.insert(stopword);
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

void KeywordProcessor::create_cn_dict(const std::string &dir, const std::string &outfile) {
    auto files = DirectoryScanner::scan(dir);
    std::map<std::string, int> wordCount;

    for (auto &file: files) {
        std::ifstream ifs{file};
        std::string line;
        while (getline(ifs, line)) {
            std::vector<std::string> words;
            tokenizer_.Cut(line, words);
            for (auto &word: words) {
                if (!is_all_chinese(word)) continue;
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
}

void KeywordProcessor::create_cn_index(const std::string &dict, const std::string &index) {
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
}

void KeywordProcessor::create_en_dict(const std::string &dir, const std::string &outfile) {
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
}

void KeywordProcessor::create_en_index(const std::string &dict, const std::string &index) {
    std::ifstream ifs{dict};
    std::string line;
    std::map<char, std::set<int> > charIndex;
    int lineNo = 0;

    while (getline(ifs, line)) {
        ++lineNo;
        std::istringstream iss{line};
        std::string word;
        int freq;
        iss >> word >> freq;

        for (auto &ch: word) charIndex[ch].insert(lineNo);
    }

    std::ofstream ofs{index};
    for (auto &[ch, indexes]: charIndex) {
        ofs << ch;
        for (int n: indexes) ofs << " " << n;
        ofs << std::endl;
    }
}
