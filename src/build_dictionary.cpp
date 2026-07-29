#include "core/EnvLoader.h"
#include "core/KeywordProcessor.h"

int main() {
    const auto corpus = EnvLoader::se_corpus_dir();
    const auto dict_output = EnvLoader::se_data_dict_dir();

    KeywordProcessor kword_proc;
    kword_proc.process(corpus, dict_output);
}