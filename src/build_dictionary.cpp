#include "core/EnvLoader.h"
#include "core/KeywordProcessor.h"

int main() {
    const auto corpus = EnvLoader::se_corpus_dir();
    const auto stopwords = EnvLoader::se_stopwords_dir();
    const auto output = EnvLoader::se_data_dir() + "/dictionary";

    KeywordProcessor processor;
    processor.process(stopwords + "/cn_stopwords.txt", stopwords + "/en_stopwords.txt");
    processor.create_cn_dict(corpus + "/CN", output + "/dict_cn.dat");
    processor.create_cn_index(output + "/dict_cn.dat", output + "/index_cn.dat");
    processor.create_en_dict(corpus + "/EN", output + "/dict_en.dat");
    processor.create_en_index(output + "/dict_en.dat", output + "/index_en.dat");
}