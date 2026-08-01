#include "server/SearchEngineBackend.h"

#include "core/EnvLoader.h"

SearchEngineBackend::SearchEngineBackend()
    : docs_(EnvLoader::se_data_webpage_dir() + "/pages.dat", EnvLoader::se_data_webpage_dir() + "/offsets.dat"),
      index_(EnvLoader::se_data_webpage_dir() + "/inverted_index.dat"),
      searcher_(docs_, index_),
      cn_dictionary_(EnvLoader::se_data_dict_dir() + "/dict_cn.dat"),
      cn_index_(EnvLoader::se_data_dict_dir() + "/index_cn.dat", cn_dictionary_.size()),
      en_dictionary_(EnvLoader::se_data_dict_dir() + "/dict_en.dat"),
      en_index_(EnvLoader::se_data_dict_dir() + "/index_en.dat", en_dictionary_.size()),
      recommender_(cn_dictionary_, cn_index_, en_dictionary_, en_index_) {}

std::string SearchEngineBackend::recommend(const std::string &query) const {
    return recommender_.recommend(query);
}

std::string SearchEngineBackend::search(const std::string &query) {
    return searcher_.search(query);
}
