#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>


class DocLibrary {
public:
    DocLibrary(const std::string &docs_path, const std::string &offsets_path);

    std::string_view get_doc(int doc_id) const;

    std::size_t size() const;

private:
    struct DocOffset {
        std::size_t offset;
        std::size_t length;
    };

    std::string docs_;
    std::unordered_map<int, DocOffset> offsets_;
};
