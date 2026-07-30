#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

class KeywordRecommender {
public:
    KeywordRecommender(const std::string &dictDir);

    // 返回的是json格式 ["aa" , "bb" , "cc" , "dd" , "ee"]
    // 没有指定的话，默认返回前5个最匹配的候选词
    // 返回格式就是一个json数组
    // 反序列化后可以直接按下标访问
    std::string recommend(const std::string &keyword, int k = 5) const;

private:
    struct Candidate {
        // 用优先队列实现候选词排序 ，这里创建一个结构体 ， 并重载小于号运算符
        // 比较逻辑：选编辑距离更小的，候选词词频高的，字典序更小的
        std::string word;
        int freq;
        int distance;

        // 此时堆顶放的是最差的匹配次
        bool operator<(const Candidate &rhs) const {
            if (distance != rhs.distance) return distance < rhs.distance;
            if (freq != rhs.freq) return freq > rhs.freq;
            return word < rhs.word;
        }
    };

    // 读取词典库
    void loadDict(const std::string &path, std::vector<std::pair<std::string, int>> &dict) const;

    // 读取索引库
    void loadIndex(const std::string &path, std::map<std::string, std::set<int>> &index) const;

    // 中文词典库和中文索引库
    std::vector<std::pair<std::string, int>> _cnDict;
    std::map<std::string, std::set<int>> _cnIndex;

    // 英文词典库和英文索引库
    std::vector<std::pair<std::string, int>> _enDict;
    std::map<std::string, std::set<int>> _enIndex;
};