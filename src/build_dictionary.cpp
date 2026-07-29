#include "core/EnvLoader.h"
#include "core/KeywordProcessor.h"
#include "webpage/PageProcessor.h"

int main() {
    const auto corpus = EnvLoader::se_corpus_dir();
    const auto raw_pages = EnvLoader::se_raw_pages_dir();
    const auto dict_output = EnvLoader::se_data_dict_dir();
    const auto webpage_output = EnvLoader::se_data_webpage_dir();

    KeywordProcessor kword_proc;
    PageProcessor page_proc;
    kword_proc.process(corpus, dict_output);
    page_proc.process(raw_pages, webpage_output);
}