#pragma once

#include <cmath>
#include <cstddef>
using std::size_t;

namespace tfidf {
    inline double weight(size_t term_freq, size_t total_terms, size_t doc_count, size_t doc_freq) {
        if (term_freq == 0 || total_terms == 0 || doc_count == 0 || doc_freq == 0)
            return 0.0;

        const double tf = static_cast<double>(term_freq) / static_cast<double>(total_terms);
        const double idf = std::log2(
            static_cast<double>(doc_count) / static_cast<double>(doc_freq) + 1.0
        );
        return tf * idf;
    }
}
