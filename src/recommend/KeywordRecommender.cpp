#include "recommend/KeywordRecommender.h"

#include <utfcpp/utf8.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <queue>
#include <sstream>
// 工具函数

// 判断该字符是不是中文
static bool isChineseChar(char32_t cp) { return cp >= 0x4E00 && cp <= 0x9FFF; }

// 判断是否全为中文
static bool isAllChinese(const std::string &s) {
    if (s.empty()) return false;
    utf8::iterator it(s.begin(), s.begin(), s.end());
    const utf8::iterator end(s.end(), s.begin(), s.end());
    for (; it != end; ++it) {
        if (!isChineseChar(*it)) return false;
    }
    return true;
}

// 状态压缩版编辑距离
// 返回的是两个单词的最小距离
static int editDistance(const std::string &a, const std::string &b) {
    // 这里选择长度更小的做列，更省空间
    const std::string &row = a.size() >= b.size() ? a : b;
    const std::string &col = a.size() >= b.size() ? b : a;
    const int m = static_cast<int>(col.size());
    std::vector<int> f(m + 1, 0);
    for (int j = 0; j < m; ++j) {
        f[j + 1] = j + 1;
    }

    for (const char x : row) {
        int pre = f[0];
        f[0]++;
        for (int j = 0; j < m; ++j) {
            int tmp = f[j + 1];
            if (col[j] == x) {
                f[j + 1] = pre;
            } else {
                f[j + 1] = std::min({f[j], f[j + 1], pre}) + 1;
            }
            pre = tmp;
        }
    }
    return f[m];
}

// 读取词典库和索引库
void KeywordRecommender::loadDict(
    const std::string &path, std::vector<std::pair<std::string, int>> &dict) const {
    std::ifstream ifs{path};
    if (!ifs) {
        std::cerr << "open file failed : " << path << std::endl;
        return;
    }
    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss{line};
        std::string word;
        int freq = 0;
        iss >> word >> freq;
        dict.emplace_back(word, freq);
    }
}

void KeywordRecommender::loadIndex(
    const std::string &path, std::map<std::string, std::set<int>> &index) const {
    std::ifstream ifs{path};
    if (!ifs) {
        std::cerr << "open file failed : " << path << std::endl;
        return;
    }
    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss{line};
        std::string word;
        iss >> word;
        int i = 0;
        while (iss >> i) {
            index[word].insert(i);
        }
    }
}

// 构造函数，在构造函数里面加载中文和英文的
// 词典库和索引库
// 这里传进来的dictDir是可以从环境变量中拿的
// SE_DATA_DIR1=./data/generated/dictionary
KeywordRecommender::KeywordRecommender(const std::string &dictDir) {
    loadDict(dictDir + "/dict_cn.dat", _cnDict);
    loadIndex(dictDir + "/index_cn.dat", _cnIndex);
    loadDict(dictDir + "/dict_en.dat", _enDict);
    loadIndex(dictDir + "/index_en.dat", _enIndex);
}

// 关键字推荐
std::string KeywordRecommender::recommend(const std::string &keyword, int k) const {
    if (keyword.empty() || k <= 0) {
        std::cerr << "keyword is empty or k <= 0" << std::endl;
        return nlohmann::json::array().dump();
    }

    // 判断keyword是不是全中文，如果是全中文，就查询中文库
    // 如果不是全中文，全英文或者中英混合，就走英文查询
    // 中英混合的情况暂时不考虑，关键先实现全中和全英的推荐
    const bool allChinese = isAllChinese(keyword);
    const auto &dict = allChinese ? _cnDict : _enDict;
    const auto &index = allChinese ? _cnIndex : _enIndex;

    // 将关键字拆分为一个一个的字符
    std::vector<std::string> characters;
    const char *curr = keyword.c_str();
    const char *end = keyword.c_str() + keyword.size();
    while (curr != end) {
        auto start = curr;
        utf8::next(curr, end);
        characters.emplace_back(start, curr);
    }

    // 查索引
    std::set<int> lines;
    for (const auto &ch : characters) {
        auto it = index.find(ch);
        if (it != index.end()) {
            lines.insert(it->second.begin(), it->second.end());
        }
    }
    if (lines.empty()) {
        return nlohmann::json::array().dump();
    }

    std::priority_queue<Candidate> prior;
    for (int line : lines) {
        int idx = line - 1;
        const auto &[word, freq] = dict[idx];
        // 结构体里面已经重载了小于号运算符
        // 也就是说现在的堆顶就是最不匹配的字符，把他删掉即可
        prior.push({word, freq, editDistance(keyword, word)});
        if (prior.size() > k) {
            prior.pop();
        }
    }
    std::vector<std::string> result;
    while (!prior.empty()) {
        result.push_back(prior.top().word);
        prior.pop();
    }
    // 这时候再逆序result , 得到的就是前k个最匹配的候选词
    std::reverse(result.begin(), result.end());
    nlohmann::json arr = result;
    return arr.dump();
}