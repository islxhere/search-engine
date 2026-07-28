#include "core/DirectoryScanner.h"
#include "core/EnvLoader.h"

#include <algorithm>
#include <dirent.h>
#include <iostream>
#include <sys/stat.h>

std::vector<std::string> DirectoryScanner::scan(const std::string &dir) {
    std::vector<std::string> files;
    DIR *dirp = opendir(dir.c_str());
    if (!dirp) {
        perror("opendir");
        return files;
    }

    dirent *entry;
    while ((entry = readdir(dirp)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        std::string fullpath = dir + "/" += name;
        struct stat st{};
        if (stat(fullpath.c_str(), &st) == 0 && S_ISREG(st.st_mode))
            files.push_back(fullpath);
    }
    closedir(dirp);
    std::sort(files.begin(), files.end());
    return files;
}
