#include "webpage/DocLibrary.h"

#include <fstream>
#include <iterator>
#include <stdexcept>


DocLibrary::DocLibrary(const std::string &docs_path, const std::string &offsets_path) {
    std::ifstream docs{docs_path, std::ios::binary};
    docs_.assign(std::istreambuf_iterator{docs}, std::istreambuf_iterator<char>{});

    std::ifstream offsets{offsets_path};
    int docid;
    DocOffset offset{};
    while (offsets >> docid >> offset.offset >> offset.length)
        offsets_.emplace(docid, offset);
}

std::string_view DocLibrary::get_doc(const int doc_id) const {
    auto it = offsets_.find(doc_id);
    if (it == offsets_.end())
        throw std::out_of_range("document not found");
    return {docs_.data() + it->second.offset, it->second.length};
}

std::size_t DocLibrary::size() const { return offsets_.size(); }
