#pragma once
#include <string>
#include <vector>

class DirectoryScanner {
public:
    DirectoryScanner() = delete;

    static std::vector<std::string> scan(const std::string &dir);

};
